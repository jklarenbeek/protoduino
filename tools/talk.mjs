#!/usr/bin/env node
// talk.mjs - talk to a protoduino system over its serial line, on a board or
// in protosim: type commands at a shell, send files with XMODEM-CRC.
//
//   node tools/talk.mjs --port /dev/ttyUSB0 [--baud 115200] <step> ...
//   node tools/talk.mjs --sim "<protosim> <firmware> [options]" <step> ...
//
// The steps are carried out in order and the conversation is printed:
//
//   <text>            type the line, wait for the prompt of the shell
//   wait:<seconds>    let time pass and collect what arrives
//   send:<file>       send the file with XMODEM-CRC, once the receiver asks
//                     for it: to the bootloader, or after "get <slot>"
//   refused:<file>    the same, for a file the receiver has to refuse
//   type:<text>       type the text as it is, wait for nothing
//   reset             pull the reset pin (protosim only)
//   power             switch the power off and on (protosim only)
//   expect:<text>     fail unless the text has arrived since the last expect
//
//   node tools/talk.mjs --port /dev/ttyUSB0 ps "get 0" send:upper-slot0.elf mods
//
// --prompt <text> names the prompt, which starts a line (default "> ");
// --timeout <seconds> is how
// long a step may take (default 30). The exit status is 0 when every step
// worked. Uses stty to set up a port: Linux and macOS.
import fs from 'node:fs';
import { openLine } from './lib/line.mjs';
import { crc16 } from './lib/elf.mjs';

const SOH = 0x01, EOT = 0x04, ACK = 0x06, NAK = 0x15, CAN = 0x18, C = 0x43;

function fail(message) {
  console.error(`\ntalk.mjs: ${message}`);
  cleanup();
  process.exit(1);
}

// ---- arguments ------------------------------------------------------------

const args = process.argv.slice(2);
let port = null, sim = null, baud = 115200, prompt = '> ', timeout = 30;
const steps = [];
for (let i = 0; i < args.length; i++) {
  if (args[i] === '--port') port = args[++i];
  else if (args[i] === '--sim') sim = args[++i];
  else if (args[i] === '--baud') baud = Number(args[++i]);
  else if (args[i] === '--prompt') prompt = args[++i];
  else if (args[i] === '--timeout') timeout = Number(args[++i]);
  else steps.push(args[i]);
}
if ((!port && !sim) || steps.length === 0) {
  console.error('usage: talk.mjs (--port <tty> [--baud <n>] | --sim "<protosim> <firmware> [options]") <step> ...\n' +
    '       steps: <text> | wait:<seconds> | send:<file> | refused:<file> | type:<text> | reset | power | expect:<text>');
  process.exit(2);
}

// ---- the line -------------------------------------------------------------

let line = null;
function cleanup() {
  if (line) line.close();
  line = null;
}
process.on('SIGINT', () => { cleanup(); process.exit(130); });

try {
  line = await openLine({ port, baud, sim });
} catch (error) {
  fail(error.message);
}

let transcript = '';                // everything received since the last expect

function show(data) {
  const text = data.toString('latin1');
  transcript += text;
  process.stdout.write(text.replace(/[^\x20-\x7e\r\n\t]/g, ''));
}

// print what arrives up to and including the marker; without one, until the time is up.
// What came after the marker stays for the next step.
async function readUntil(marker, seconds) {
  const end = Date.now() + seconds * 1000;
  let seen = '';
  for (;;) {
    if (line.pending.length) {
      const before = seen.length;
      seen += line.pending.toString('latin1');
      const at = marker ? seen.indexOf(marker) : -1;
      const take = at < 0 ? line.pending.length : at + marker.length - before;
      show(line.pending.subarray(0, take));
      line.pending = line.pending.subarray(take);
      if (at >= 0) return true;
    }
    const left = end - Date.now();
    if (left <= 0) return false;
    await line.arrived(Math.min(left, 50));
  }
}

const byte = (seconds) => line.byte(seconds);
const type = (data) => line.write(data);

// ---- XMODEM-CRC -------------------------------------------------------------

async function sendFile(file, refusal) {
  const data = fs.readFileSync(file);
  // the receiver asks with 'C'; whatever comes before it belongs to the conversation
  for (;;) {
    const value = await byte(timeout);
    if (value < 0) fail(`nobody asked for ${file}`);
    if (value === C) break;
    show(Buffer.from([value]));
  }
  const blocks = Math.ceil(data.length / 128);
  for (let n = 1; n <= blocks; n++) {
    const block = Buffer.alloc(128, 0x1a);
    data.copy(block, 0, (n - 1) * 128, n * 128);
    const frame = Buffer.concat([Buffer.from([SOH, n & 0xff, ~n & 0xff]), block, Buffer.from([crc16(block) >> 8, crc16(block) & 0xff])]);
    let tries = 0;
    for (;;) {
      type(frame);
      let reply = await byte(timeout);
      while (reply === C) reply = await byte(timeout);   // a request that was already on its way
      if (reply === ACK) break;
      if (reply === CAN) {
        let reason = await byte(1);
        while (reason === CAN) reason = await byte(1);   // the reason follows the cancel bytes
        const how = `${file} was refused at block ${n} of ${blocks}` + (reason >= 0 ? ` (error 0x${reason.toString(16)})` : '');
        if (!refusal) fail(how);
        process.stdout.write(`[${how}]`);
        return;
      }
      if (++tries === 10) fail(`block ${n} of ${file} was never acknowledged`);
    }
  }
  type(Buffer.from([EOT]));
  if (await byte(timeout) !== ACK) fail(`the end of ${file} was not acknowledged`);
  if (refusal) fail(`${file} was accepted`);
  process.stdout.write(`[sent ${file}: ${data.length} bytes, ${blocks} blocks]`);
}

// ---- the steps ------------------------------------------------------------

let greeted = false;
for (const step of steps) {
  if (step.startsWith('wait:')) {
    await readUntil(null, Number(step.slice(5)));
  } else if (step.startsWith('send:')) {
    await sendFile(step.slice(5), false);
  } else if (step.startsWith('refused:')) {
    await sendFile(step.slice(8), true);
  } else if (step.startsWith('type:')) {
    type(step.slice(5));
  } else if (step === 'reset' || step === 'power') {
    if (!line.child) fail(`${step}: only protosim can be reset from here`);
    line.signal(step === 'reset' ? 'SIGUSR1' : 'SIGUSR2');
    greeted = false;
  } else if (step.startsWith('expect:')) {
    await readUntil(null, 0.2);
    if (!transcript.includes(step.slice(7))) fail(`expected "${step.slice(7)}"`);
    transcript = '';
  } else {
    if (!greeted) {
      // an empty line: the shell answers with its prompt
      greeted = true;
      type('\r');
      if (!await readUntil(`\n${prompt}`, timeout)) fail('no prompt');
    }
    // a line that hands the line to somebody else gets no prompt before that somebody is done
    const next = steps[steps.indexOf(step) + 1] ?? '';
    type(`${step}\r`);
    if (!/^(send|refused):/.test(next) && !await readUntil(`\n${prompt}`, timeout)) fail(`no prompt after "${step}"`);
  }
}
await readUntil(null, 0.2);
process.stdout.write('\n');
cleanup();
process.exit(0);
