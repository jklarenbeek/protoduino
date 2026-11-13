#!/usr/bin/env node
// port.mjs - the shell of a protoduino system, on the PC.
//
//   node tools/port.mjs <sketch.elf> --port /dev/ttyUSB0 [--baud 9600] <command> ...
//   node tools/port.mjs <sketch.elf> --sim "<protosim> <sketch.elf> [options]" <command> ...
//
// The part runs a port (src/mod/port.h): it reads memory, writes memory and
// calls functions when asked. Every name and address comes from the .elf
// file of the sketch, which has to be the one the part runs.
//
//   ps                 processes: name, state, priority
//                      I started  R running  W waiting  P paused  X ending
//   mods               modules the sketch lists: number, name, + when running
//   ipc                connections: "ticker > blink"
//   log                what the kernel logged: name, severity, event, code
//   stop <name>        pause a process            cont <name>   let it continue
//   kill <name>        end a process              run <number>  start a module
//   join <a> <b>       connect the output of a to the input of b
//   unjoin <a>         take the output of a away
//   write <name> text  write text to the input of a process
//   settings           the settings of the sketch, as they are and as built
//   set <name> <value> change a setting; it is kept in EEPROM
//   <view>             a view the sketch defines; "views" lists them
//   <action>           an action the sketch defines; "actions" lists them
//   door               how far the port opens, and whether it is open now
//   get <slot> <file>  put a module into a flash slot and start it (level 4)
//   del <slot>         end the module of a slot and take it out (level 4)
//   raw <op> <n> <a> [byte ...]       one request as it is; prints its status
//   read <symbol|address> [count]     memory, as hex
//   poke <symbol|address> <byte> ...  write memory
//   eeprom [address [count]]          EEPROM, as hex
//
// For scripts: wait:<seconds>, expect:<text> (fail unless the text has been
// printed since the last expect), reset and power (protosim only).
// Without a command the tool reads commands from its input.
//
// --gap <ms> is the pause between two bytes sent to the part (default 2 on
// a port: a line on two pins takes a byte at a time). --timeout <seconds>
// is how long an answer may take (default 3). --sim-log <file> keeps what
// protosim prints.
import fs from 'node:fs';
import readline from 'node:readline';
import { openLine } from './lib/line.mjs';
import { readElf, avrImage, crc16 } from './lib/elf.mjs';

const CHECK = 0xa5;
const STATES = '?IRWXP??';
const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

let line = null;
function fail(message) {
  console.error(`port.mjs: ${message}`);
  if (line) line.close();
  process.exit(1);
}

// ---- arguments ------------------------------------------------------------

const args = process.argv.slice(2);
let port = null, sim = null, simLog = null, baud = 9600, gap = null, timeout = 3;
const plain = [];
for (let i = 0; i < args.length; i++) {
  if (args[i] === '--port') port = args[++i];
  else if (args[i] === '--sim') sim = args[++i];
  else if (args[i] === '--sim-log') simLog = args[++i];
  else if (args[i] === '--baud') baud = Number(args[++i]);
  else if (args[i] === '--gap') gap = Number(args[++i]);
  else if (args[i] === '--timeout') timeout = Number(args[++i]);
  else plain.push(args[i]);
}
if (plain.length < 1 || (!port && !sim)) {
  console.error('usage: port.mjs <sketch.elf> (--port <tty> [--baud <n>] | --sim "<protosim> <sketch.elf> [options]") [<command> ...]');
  process.exit(2);
}
const [file, ...commands] = plain;
gap = gap ?? (sim ? 0 : 2); // protosim spaces the bytes itself

// ---- what the file knows --------------------------------------------------

let elf, image;
try {
  elf = readElf(file);
  image = avrImage(elf);
} catch (error) {
  fail(error.message);
}

// a symbol by its name; link-time optimisation may have added a suffix to a local one
// and a variable of a sketch that is local to its file carries its length in front: _ZL10blink_proc
function symbol(name) {
  const local = `_ZL${name.length}${name}`;
  const found = elf.symbols.filter((s) => s.section !== 0
    && (s.name === name || s.name.startsWith(`${name}.`) || s.name === local || s.name.startsWith(`${local}.`)));
  return found.find((s) => s.name === name) ?? found[0] ?? null;
}
const ram = (s) => s.value & 0xffff;                 // data addresses carry 0x800000 in the file
const flashAt = (address, count) => image.flash.subarray(address - image.base, address - image.base + count);
function flashText(address) {
  if (!address) return '?';
  const at = address - image.base;
  return image.flash.toString('latin1', at, image.flash.indexOf(0, at));
}

const layoutSymbol = symbol('port_layout');
if (!layoutSymbol) fail(`${file} has no port: the sketch has to include <mod/port.h>`);
const LAYOUT = 17;
const layoutBytes = flashAt(layoutSymbol.value, LAYOUT);
if (layoutBytes[0] !== 2) fail(`${file} was built with another version of the port than this tool knows`);
const L = {
  level: layoutBytes[1], process: layoutBytes[2], next: layoutBytes[3], name: layoutBytes[4], prio: layoutBytes[5],
  thread: layoutBytes[6], pipein: layoutBytes[7], pipeout: layoutBytes[8], pipe: layoutBytes[9], wake: layoutBytes[10],
  scratch: layoutBytes[11], scratchSize: layoutBytes[12], guard: layoutBytes[13], window: layoutBytes[14],
  page: layoutBytes[15] * 2, actions: layoutBytes[16],
};
const LEVELS = ['', 'look', 'settings', 'work', 'load'];

// the flash slots of the sketch, if it has them (src/sys/module.h)
const slotsSymbol = symbol('module_layout');
const SLOTS = slotsSymbol ? (() => {
  const b = flashAt(slotsSymbol.value, 20);
  const [base, size, count, ramSize] = [0, 4, 8, 12].map((o) => b.readUInt32LE(o));
  return { base, size, count, ramSize, table: base + count * size, kernel: crc16(image.flash) };
})() : null;

// the notes of the sketch: settings and views, kept in the file and not in the part
const notes = [];
const noteSection = elf.section('.protoduino.notes');
if (noteSection) {
  for (const text of elf.bytes(noteSection).toString('latin1').split('\0')) if (text) notes.push(text.split('\t'));
}

// ---- the port ---------------------------------------------------------------

async function send(frame) {
  if (gap <= 0) { line.write(frame); return; }
  for (const value of frame) {
    line.write(Buffer.from([value]));
    await sleep(gap);
  }
}

// what a request needs, for the messages
const NEEDS = { R: 1, F: 1, E: 1, I: 1, e: 2, A: 2, W: 3, C: 3, B: 4, L: 4, G: 4, T: 4 };

// why the part said no
function refusal(op, status) {
  if (status === 1) {
    return `the part was built with PORT_CONF_LEVEL ${L.level} (${LEVELS[L.level]}); this needs level ${NEEDS[op]} (${LEVELS[NEEDS[op]]})`;
  }
  if (status === 2) {
    return L.guard === 2
      ? 'levels 3 and 4 are closed: port_may_open() of the sketch says so'
      : `levels 3 and 4 are closed. Reset the part through its reset pin (a button, or "reset" here), then go on within ${L.window} s`;
  }
  if (status === 3) return 'that is outside what this request may touch';
  if (status === 4) return 'the slot holds a module, or its module still runs: "del" it first';
  return 'the part does not know this request';
}

class Refused extends Error {
  constructor(op, status) {
    super(refusal(op, status));
    this.status = status;
  }
}

// one request; returns the bytes of the answer, or throws Refused
async function request(op, n, a, d = []) {
  const frame = Buffer.alloc(12);
  frame[0] = 0x70; frame[1] = op.charCodeAt(0); frame[2] = n; frame.writeUInt16LE(a & 0xffff, 3);
  Buffer.from(d).copy(frame, 5);
  frame[11] = frame.subarray(0, 11).reduce((x, value) => x ^ value, 0);
  const length = 'RFE'.includes(op) ? n : op === 'I' ? 3 : op === 'C' ? 2 : op === 'T' ? 1 : 0;

  for (let attempt = 0; attempt < 3; attempt++) {
    line.take();
    await send(frame);
    const status = await line.byte(timeout);
    const answer = [status];
    if (status >= 0) {
      const expected = (status === 0 ? length : 0) + 2;
      while (answer.length < expected) {
        const value = await line.byte(timeout);
        if (value < 0) break;
        answer.push(value);
      }
      if (answer.length === expected && answer.reduce((x, value) => x ^ value, CHECK) === 0) {
        if (status !== 0) throw new Refused(op, status);
        return Buffer.from(answer.slice(1, -1));
      }
    }
    await sleep(100); // let the line fall silent, then once more
  }
  fail(`the part does not answer (${op} at 0x${a.toString(16)}). Is it the port of this file, on this line?`);
}

async function read(space, address, count) {
  const parts = [];
  for (let done = 0; done < count; done += 32) {
    parts.push(await request(space, Math.min(32, count - done), address + done));
  }
  return Buffer.concat(parts);
}
const read8 = async (address) => (await read('R', address, 1))[0];
const read16 = async (address) => (await read('R', address, 2)).readUInt16LE(0);

async function write(space, address, data) {
  for (let done = 0; done < data.length; done += 6) {
    const chunk = data.subarray(done, done + 6);
    await request(space, chunk.length, address + done, chunk);
  }
}

async function call(name, a = 0, b = 0, c = 0) {
  if (L.level < 3) throw new Refused('C', 1);
  const fn = symbol(name);
  if (!fn) throw new Error(`the sketch does not contain ${name}: name it in PORT_CONF_CALLS`);
  const d = Buffer.alloc(6);
  d.writeUInt16LE(a & 0xffff, 0); d.writeUInt16LE(b & 0xffff, 2); d.writeUInt16LE(c & 0xffff, 4);
  return (await request('C', 0, fn.value >> 1, d)).readUInt16LE(0);
}

// ---- processes --------------------------------------------------------------

function decode(address, bytes) {
  const flags = bytes[L.prio + 1];
  return {
    address,
    next: bytes.readUInt16LE(L.next),
    name: L.name === 0xff ? `@${address.toString(16)}` : null,
    nameAt: L.name === 0xff ? 0 : bytes.readUInt16LE(L.name),
    prio: bytes[L.prio],
    state: flags & 7, device: ((flags >> 3) & 3) === 1, hasIn: (flags >> 6) & 1, hasOut: (flags >> 7) & 1,
    pipein: bytes.readUInt16LE(L.pipein), pipeout: bytes.readUInt16LE(L.pipeout),
  };
}

// a name in flash: in the file for the sketch itself, on the part for a module in a slot
async function nameAt(address) {
  if (address >= image.base && address < image.base + image.flash.length) return flashText(address);
  const text = await read('F', address, 12);
  return text.toString('latin1', 0, Math.max(0, text.indexOf(0)));
}

async function processes() {
  const head = symbol('process_list');
  if (!head) throw new Error('the file has no process list');
  const list = [];
  for (let p = await read16(ram(head)); p !== 0 && list.length < 32;) {
    const process = decode(p, await read('R', p, L.process));
    if (process.name === null) process.name = await nameAt(process.nameAt);
    list.push(process);
    p = process.next;
  }
  return list;
}

async function named(name) {
  const found = (await processes()).find((p) => p.name === name);
  if (!found) throw new Error(`nobody by the name ${name}`);
  return found;
}

// the pipe a process writes into, and the one that is written to reach it.
// A device publishes them the other way round.
const pipeOf = (p, into) => (p.device !== into ? (p.hasOut ? p.pipeout : 0) : (p.hasIn ? p.pipein : 0));

function modules() {
  const table = symbol('port_modules');
  if (!table) return [];
  const bytes = flashAt(table.value, table.size);
  const list = [];
  for (let o = 0; o + 4 <= bytes.length; o += 4) {
    const type = bytes.readUInt16LE(o);
    list.push({ type, instance: bytes.readUInt16LE(o + 2), name: flashText(flashAt(type, 2).readUInt16LE(0)) });
  }
  return list;
}

// ---- settings and views -----------------------------------------------------

// a number the sketch wrote as an expression of numbers
function value(text) {
  if (!/^[\s0-9a-fA-FxX+\-*/()UL]*$/.test(text)) throw new Error(`"${text}" is not a number`);
  return Number(Function(`"use strict"; return (${text.replace(/[UL]+\b/g, '')});`)()) | 0;
}

function settings() {
  // setting <name> <help>, in the order of their bytes; "settings <EEPROM address>" comes first
  const base = notes.find((n) => n[0] === 'settings');
  if (!base) return null;
  const list = notes.filter((n) => n[0] === 'setting').map((n, i) => ({ name: n[1], help: n[2] ?? '', index: i }));
  const defaults = symbol('settings_defaults');
  const builtIn = defaults ? flashAt(defaults.value, list.length) : Buffer.alloc(list.length);
  for (const s of list) s.default = builtIn[s.index];
  return { at: value(base[1]), list };
}

const SETTINGS_MARK = 0x5e;
async function settingsNow(all) {
  const stored = await read('E', all.at, all.list.length + 1);
  const valid = stored[all.list.length] === ((SETTINGS_MARK ^ all.list.length) & 0xff);
  return all.list.map((s) => (valid ? stored[s.index] : s.default));
}

// how a view shows a sample
const UNITS = {
  raw: (x) => String(x),
  tenths: (x) => `${(x / 10).toFixed(1)} V`,                    // tenths of a volt
  celsius: (x) => `${x - 40} C`,                                // the sample is degrees above -40
  percent: (x) => `${Math.round(x * 100 / 255)} %`,
};

// ---- flash slots ------------------------------------------------------------

// what the slots hold: read from the part, not from the file
async function slotList() {
  if (!SLOTS) return [];
  const table = await read('F', SLOTS.table, 2 + 2 * SLOTS.count);
  const valid = table.readUInt16LE(0) === 0x4d70;
  const ram = symbol('module_ram');
  const list = [];
  for (let slot = 0; slot < SLOTS.count; slot++) {
    const x = { slot, name: null, running: false };
    if (valid && table.readUInt16LE(2 + 2 * slot) === SLOTS.kernel) {
      const at = SLOTS.base + slot * SLOTS.size;
      const type = (await read('F', at + 6, 2)).readUInt16LE(0);
      const name = (await read('F', type, 2)).readUInt16LE(0);
      const text = await read('F', name, 12);
      x.name = text.toString('latin1', 0, Math.max(0, text.indexOf(0)));
      if (ram) x.running = ((await read8(ram.value + slot * SLOTS.ramSize + L.prio + 1)) & 7) !== 0;
    }
    list.push(x);
  }
  return list;
}

// a module into a slot: page by page, each read back, then checked and started by the part
async function get(slot, moduleFile) {
  if (!SLOTS) throw new Error('the sketch has no flash slots (sys/module.h)');
  if (L.level < 4) throw new Refused('B', 1);
  if (!(slot >= 0 && slot < SLOTS.count)) throw new Error(`no slot ${slot}: the sketch has 0 to ${SLOTS.count - 1}`);
  let module;
  try {
    module = readElf(moduleFile);
  } catch (error) {
    throw new Error(error.message);
  }
  const text = module.section('.text');
  const at = SLOTS.base + slot * SLOTS.size;
  if (!text || !module.symbols.find((x) => x.name === 'module_descriptor')) throw new Error(`${moduleFile} is not a module: build it with tools/module.mjs`);
  if (text.addr !== at) throw new Error(`${moduleFile} is linked for 0x${text.addr.toString(16)}, slot ${slot} is at 0x${at.toString(16)}: build it for this slot`);
  const bytes = module.bytes(text);
  if (bytes.length > SLOTS.size) throw new Error(`${moduleFile} takes ${bytes.length} bytes, a slot holds ${SLOTS.size}`);
  if (bytes.readUInt16LE(4) !== SLOTS.kernel) throw new Error(`${moduleFile} is linked against another build of this sketch: build it again from ${file}`);

  const pages = Math.ceil(bytes.length / L.page);
  for (let p = 0; p < pages; p++) {
    const page = at + p * L.page;
    const data = Buffer.alloc(L.page, 0xff);
    bytes.copy(data, 0, p * L.page, (p + 1) * L.page);
    let done = false;
    for (let attempt = 0; attempt < 3 && !done; attempt++) {
      // a module that was just ended may take a turn to let go of its slot
      for (let wait = 0; ; wait++) {
        try {
          await request('B', 0, page);
          break;
        } catch (error) {
          if (!(error instanceof Refused) || error.status !== 4 || wait === 10) throw error;
          await sleep(100);
        }
      }
      for (let w = 0; w < L.page; w += 6) {
        const chunk = data.subarray(w, Math.min(w + 6, L.page));
        await request('L', chunk.length / 2, w, chunk);
      }
      await request('G', 0, 0);
      done = (await read('F', page, L.page)).equals(data);
    }
    if (!done) {
      throw new Error(`page 0x${page.toString(16)} did not take. Is the SELFPRGEN fuse programmed? ("door" says)`);
    }
  }
  say(`[sent ${moduleFile}: ${bytes.length} bytes, ${pages} pages]`);
  const code = (await request('T', 1, slot))[0];
  const why = { 0x51: 'not a module', 0xd0: 'another version of the module format', 0xa4: 'linked against another build', 0xa6: 'the slot is in use', 0x20: 'its process struct does not fit the RAM of a slot' };
  say(code === 0 ? 'ok' : `? the part refused the module: ${why[code] ?? `code 0x${hex([code])}`}`);
}

// ---- the commands -----------------------------------------------------------

let transcript = '';
function say(text) {
  transcript += `${text}\n`;
  console.log(text);
}

const hex = (bytes) => [...bytes].map((x) => x.toString(16).padStart(2, '0')).join(' ');

async function command(text) {
  const words = text.trim().split(/\s+/);
  const c = words[0];

  if (c === 'ps') {
    for (const p of await processes()) say(`${p.name.padEnd(8)} ${STATES[p.state]} ${p.prio}`);
  } else if (c === 'mods') {
    // the slots first, as the shell on a part lists them: the number of a slot is its number
    const slots = await slotList();
    for (const x of slots) say(`${x.slot} ${x.name ?? '(free)'}${x.name ? (x.running ? ' +' : ' -') : ''}`);
    const list = modules();
    for (let i = 0; i < list.length; i++) {
      const flags = await read8(list[i].instance + L.prio + 1);
      say(`${slots.length + i} ${list[i].name} ${(flags & 7) !== 0 ? '+' : '-'}`);
    }
  } else if (c === 'ipc') {
    const list = await processes();
    for (const p of list) {
      const pipe = pipeOf(p, true);
      if (!pipe) continue;
      // whoever a pipe wakes is its reader, also when that process has more inputs than one
      const wake = (await read('R', pipe + L.wake, 2)).readUInt16LE(0);
      const reader = list.find((q) => q.address === wake);
      if (reader && reader !== p) say(`${p.name} > ${reader.name}`);
    }
  } else if (c === 'log') {
    const pool = symbol('error_pool'), index = symbol('error_pool_idx');
    if (!pool || !index) throw new Error('the file has no log');
    const count = pool.size / 5, entries = await read('R', ram(pool), pool.size), newest = await read8(ram(index));
    const list = await processes();
    for (let age = 0; age < count; age++) {
      const e = entries.subarray(((newest - 1 - age + 256 * count) % count) * 5);
      if ((e[2] & 7) === 0) break;
      const source = list.find((p) => p.address === e.readUInt16LE(0));
      say(`${source ? source.name : '?'} ${e[2] & 7} ${hex([e[3]])} ${hex([e[4]])}`);
    }
  } else if (c === 'stop' || c === 'cont') {
    const p = await named(words[1]);
    say(await call(c === 'stop' ? 'process_pause' : 'process_resume', p.address) ? 'ok' : '?');
  } else if (c === 'kill') {
    const p = await named(words[1]);
    if (p.device || p.name === 'port') { say('?'); return; }
    await call('process_resume', p.address);
    await call('process_destroy', p.address);
    say('ok');
  } else if (c === 'run') {
    const count = SLOTS ? SLOTS.count : 0;
    if (Number(words[1]) < count) {
      const code = (await request('T', 2, Number(words[1])))[0];
      say(code === 0 ? 'ok' : `? the slot does not start: 0x${hex([code])}`);
      return;
    }
    const list = modules();
    const m = list[Number(words[1]) - count] ?? list.find((x) => x.name === words[1]);
    if (!m || ((await read8(m.instance + L.prio + 1)) & 7) !== 0) { say('?'); return; }
    say(await call('process_new', m.type, m.instance) ? 'ok' : '?');
  } else if (c === 'join') {
    say(await call('process_connect', (await named(words[1])).address, (await named(words[2])).address) ? 'ok' : '?');
  } else if (c === 'unjoin') {
    await call('process_disconnect', (await named(words[1])).address);
    say('ok');
  } else if (c === 'write') {
    const p = await named(words[1]);
    const pipe = pipeOf(p, false);
    const data = Buffer.from(text.trim().split(/\s+/).slice(2).join(' '), 'latin1');
    if (!pipe || data.length === 0) { say('?'); return; }
    const scratch = ram(symbol('port_proc')) + L.scratch;
    for (let done = 0; done < data.length; done += L.scratchSize) {
      const chunk = data.subarray(done, done + L.scratchSize);
      await write('W', scratch, chunk);
      await call('ipc_pipe_write', pipe, scratch, chunk.length);
    }
    say('ok');
  } else if (c === 'settings') {
    const all = settings();
    if (!all) throw new Error('the sketch has no settings');
    const now = await settingsNow(all);
    for (const s of all.list) say(`${s.name.padEnd(10)} ${String(now[s.index]).padStart(3)}   (${String(s.default).padStart(3)})  ${s.help}`);
  } else if (c === 'set') {
    const all = settings();
    const s = all?.list.find((x) => x.name === words[1]);
    const v = Number(words[2]);
    if (!s || !Number.isInteger(v) || v < 0 || v > 255) { say('?'); return; }
    const now = await settingsNow(all);
    now[s.index] = v;
    // all of them and the mark behind them: from now on EEPROM says what the settings are
    await write('e', all.at, Buffer.from([...now, (SETTINGS_MARK ^ all.list.length) & 0xff]));
    await request('A', 0xff, 0); // and the sketch takes them over
    say('ok');
  } else if (c === 'actions') {
    for (const n of notes.filter((x) => x[0] === 'action')) say(`${n[1].padEnd(10)} ${n[2] ?? ''}`);
  } else if (c === 'door') {
    const [open, cause, fuse] = await request('I', 0, 0);
    const causes = ['power-on', 'reset pin', 'brown-out', 'watchdog'].filter((_, i) => cause & (1 << i)).join(', ') || 'none';
    say(`level ${L.level} (${LEVELS[L.level]})`);
    if (L.level >= 3) {
      say(`levels 3 and 4: ${open ? 'open' : 'closed'}, ${['always open', `open for ${L.window} s after a reset through the pin`, 'open when the sketch says so'][L.guard]}`);
    }
    say(`the last reset: ${causes}`);
    if (L.level >= 4) say(`SELFPRGEN fuse: ${fuse & 1 ? 'not programmed: the part cannot program its flash' : 'programmed'}`);
  } else if (c === 'raw') {
    // one request as it is, for tests of what the part itself allows: raw <op> <n> <a> [<byte> ...]
    try {
      const answer = await request(words[1], Number(words[2]), Number(words[3]), words.slice(4).map(Number));
      say(`status 0 ${hex(answer)}`);
    } catch (error) {
      if (!(error instanceof Refused)) throw error;
      say(`status ${error.status}`);
    }
  } else if (c === 'get') {
    await get(Number(words[1]), words[2]);
  } else if (c === 'del') {
    const slot = Number(words[1]);
    if (!SLOTS || !(slot >= 0 && slot < SLOTS.count)) throw new Error(`no slot ${words[1]}`);
    const code = (await request('T', 0, slot))[0];
    say(code === 0 ? 'ok' : `? 0x${hex([code])}`);
  } else if (c === 'views') {
    for (const n of notes.filter((x) => x[0] === 'view')) say(`${n[1].padEnd(10)} ${n[3] ?? ''}`);
  } else if (c === 'read') {
    const s = symbol(words[1]);
    const address = s ? ram(s) : Number(words[1]);
    if (!Number.isInteger(address)) throw new Error(`no symbol ${words[1]}`);
    say(hex(await read('R', address, Number(words[2] ?? (s && s.size) ?? 1) || 1)));
  } else if (c === 'poke') {
    const target = symbol(words[1]);
    await write('W', target ? ram(target) : Number(words[1]), Buffer.from(words.slice(2).map(Number)));
    say('ok');
  } else if (c === 'eeprom') {
    say(hex(await read('E', Number(words[1] ?? 0), Number(words[2] ?? 16))));
  } else if (c === 'ring' || c === 'byte') {
    await view(words);
  } else {
    const actions = notes.filter((x) => x[0] === 'action');
    const index = actions.findIndex((x) => x[1] === c);
    if (index >= 0) {
      await request('A', index, 0);
      say('ok');
      return;
    }
    const n = notes.find((x) => x[0] === 'view' && x[1] === c);
    if (!n) throw new Error(`no command ${c}`);
    for (const part of n[2].split(';')) await view(words_of(part));
  }
}

// the words of a view; a number may be written as an expression in brackets, with spaces in it
function words_of(text) {
  const words = [];
  let depth = 0, word = '';
  for (const ch of text.trim()) {
    if (ch === '(') depth++;
    if (ch === ')') depth--;
    if (/\s/.test(ch) && depth === 0) {
      if (word) words.push(word);
      word = '';
    } else {
      word += ch;
    }
  }
  if (word) words.push(word);
  return words;
}

// what a view is made of:
//   byte <e|r> <address> <unit> <label>      one byte of EEPROM or memory
//   ring <address> <count> <unit>            a record of mod/record.h in EEPROM: lowest, highest, samples
async function view(words) {
  if (words[0] === 'byte') {
    const x = (await read(words[1] === 'e' ? 'E' : 'R', value(words[2]), 1))[0];
    // a counter in EEPROM that was never written reads as erased: nothing counted
    const shown = words[1] === 'e' && words[3] === 'raw' && x === 0xff ? 0 : x;
    say(`${words.slice(4).join(' ')}: ${(UNITS[words[3]] ?? UNITS.raw)(shown)}`);
  } else if (words[0] === 'ring') {
    const at = value(words[1]), count = value(words[2]), unit = UNITS[words[3]] ?? UNITS.raw;
    const bytes = await read('E', at, count + 2);
    const samples = [...bytes.subarray(2)];
    const end = samples.indexOf(0xff);            // the mark behind the newest sample
    const inOrder = end < 0 ? samples : [...samples.slice(end + 1), ...samples.slice(0, end)].filter((x) => x !== 0xff);
    if (inOrder.length === 0) { say('nothing recorded yet'); return; }
    say(`lowest ${unit(bytes[0])}, highest ${unit(bytes[1])}, ${inOrder.length} samples, oldest first:`);
    say(inOrder.map(unit).join('  '));
  }
}

// ---- run ----------------------------------------------------------------------

try {
  line = await openLine({ port, baud, sim, simLog });
} catch (error) {
  fail(error.message);
}
process.on('SIGINT', () => { line.close(); process.exit(130); });

// is this the build the part runs? Its layout record has to be where the file says
const there = await read('F', layoutSymbol.value, LAYOUT);
if (!there.equals(Buffer.from(layoutBytes))) fail(`the part does not run ${file}: build and upload again, or name the right file`);

async function step(text) {
  if (text.startsWith('wait:')) {
    await sleep(Number(text.slice(5)) * 1000);
  } else if (text.startsWith('expect:')) {
    if (!transcript.includes(text.slice(7))) fail(`expected "${text.slice(7)}"`);
    transcript = '';
  } else if (text === 'reset' || text === 'power') {
    line.signal(text === 'reset' ? 'SIGUSR1' : 'SIGUSR2');
    await sleep(300);
  } else {
    try {
      await command(text);
    } catch (error) {
      say(`? ${error.message}`);
      if (error instanceof TypeError || error instanceof ReferenceError || error instanceof RangeError) throw error; // a fault of this tool
    }
  }
}

if (commands.length) {
  for (const text of commands) await step(text);
} else {
  const input = readline.createInterface({ input: process.stdin, output: process.stdout, prompt: '> ' });
  input.prompt();
  for await (const text of input) {
    if (text.trim()) await step(text);
    input.prompt();
  }
}
line.close();
process.exit(0);
