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
import { readElf, avrImage } from './lib/elf.mjs';

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
const layoutBytes = flashAt(layoutSymbol.value, 13);
if (layoutBytes[0] !== 1) fail(`${file} was built with another version of the port than this tool knows`);
const L = {
  level: layoutBytes[1], process: layoutBytes[2], next: layoutBytes[3], name: layoutBytes[4], prio: layoutBytes[5],
  thread: layoutBytes[6], pipein: layoutBytes[7], pipeout: layoutBytes[8], pipe: layoutBytes[9], wake: layoutBytes[10],
  scratch: layoutBytes[11], scratchSize: layoutBytes[12],
};

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

// one request; returns the bytes of the answer
async function request(op, n, a, d = []) {
  const frame = Buffer.alloc(12);
  frame[0] = 0x70; frame[1] = op.charCodeAt(0); frame[2] = n; frame.writeUInt16LE(a & 0xffff, 3);
  Buffer.from(d).copy(frame, 5);
  frame[11] = frame.subarray(0, 11).reduce((x, value) => x ^ value, 0);
  const expected = (op === 'W' || op === 'e' ? 0 : op === 'C' ? 2 : n) + 1;

  for (let attempt = 0; attempt < 3; attempt++) {
    line.take();
    await send(frame);
    const answer = [];
    while (answer.length < expected) {
      const value = await line.byte(timeout);
      if (value < 0) break;
      answer.push(value);
    }
    if (answer.length === expected && answer.reduce((x, value) => x ^ value, CHECK) === 0) {
      return Buffer.from(answer.slice(0, -1));
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
  if (L.level < 2) throw new Error('this port only looks (PORT_CONF_LEVEL 1)');
  for (let done = 0; done < data.length; done += 6) {
    const chunk = data.subarray(done, done + 6);
    await request(space, chunk.length, address + done, chunk);
  }
}

async function call(name, a = 0, b = 0, c = 0) {
  if (L.level < 3) throw new Error('this port does not call functions (PORT_CONF_LEVEL is below 3)');
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
    name: L.name === 0xff ? `@${address.toString(16)}` : flashText(bytes.readUInt16LE(L.name)),
    prio: bytes[L.prio],
    state: flags & 7, device: ((flags >> 3) & 3) === 1, hasIn: (flags >> 6) & 1, hasOut: (flags >> 7) & 1,
    pipein: bytes.readUInt16LE(L.pipein), pipeout: bytes.readUInt16LE(L.pipeout),
  };
}

async function processes() {
  const head = symbol('process_list');
  if (!head) throw new Error('the file has no process list');
  const list = [];
  for (let p = await read16(ram(head)); p !== 0 && list.length < 32;) {
    const process = decode(p, await read('R', p, L.process));
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
    const list = modules();
    for (let i = 0; i < list.length; i++) {
      const flags = await read8(list[i].instance + L.prio + 1);
      say(`${i} ${list[i].name} ${(flags & 7) !== 0 ? '+' : '-'}`);
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
    const list = modules();
    const m = list[Number(words[1])] ?? list.find((x) => x.name === words[1]);
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
    if (symbol('settings_apply') && L.level >= 3) await call('settings_apply');
    say('ok');
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
const there = await read('F', layoutSymbol.value, 13);
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
