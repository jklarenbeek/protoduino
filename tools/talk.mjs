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
import { execFileSync, spawn } from 'node:child_process';
import fs from 'node:fs';

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

let child = null, fd = -1;
function cleanup() {
  if (child) child.kill();
  child = null;
}
process.on('SIGINT', () => { cleanup(); process.exit(130); });

const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

if (sim) {
  // protosim makes a pseudo terminal for UART0 and links it here
  const link = '/tmp/simavr-uart0';
  fs.rmSync(link, { force: true });
  const [command, ...rest] = sim.split(/\s+/).filter(Boolean);
  child = spawn(command, rest, { stdio: 'ignore' });
  child.on('error', (error) => fail(`cannot start ${command}: ${error.message}`));
  for (let i = 0; i < 100 && !fs.existsSync(link); i++) await sleep(50);
  if (!fs.existsSync(link)) fail('protosim did not open its serial line');
  await sleep(200);
  port = fs.realpathSync(link);
  baud = null; // a pseudo terminal has no rate
}
try {
  const flag = process.platform === 'darwin' ? '-f' : '-F';
  execFileSync('stty', [flag, port, ...(baud ? [String(baud)] : []), 'raw', '-echo'], { stdio: 'ignore' });
} catch (error) {
  fail(`cannot set up ${port}: ${error.message}`);
}
fd = fs.openSync(port, fs.constants.O_RDWR | fs.constants.O_NOCTTY);

let pending = Buffer.alloc(0);      // received, not yet looked at
let transcript = '';                // everything received since the last expect
let waiting = null;
fs.createReadStream(null, { fd, autoClose: false, highWaterMark: 256 })
  .on('data', (data) => {
    pending = Buffer.concat([pending, data]);
    if (waiting) waiting();
  })
  .on('error', () => {});

function arrived(ms) {
  return new Promise((resolve) => {
    const timer = setTimeout(() => { waiting = null; resolve(false); }, ms);
    waiting = () => { clearTimeout(timer); waiting = null; resolve(true); };
  });
}

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
    if (pending.length) {
      const before = seen.length;
      seen += pending.toString('latin1');
      const at = marker ? seen.indexOf(marker) : -1;
      const take = at < 0 ? pending.length : at + marker.length - before;
      show(pending.subarray(0, take));
      pending = pending.subarray(take);
      if (at >= 0) return true;
    }
    const left = end - Date.now();
    if (left <= 0) return false;
    await arrived(Math.min(left, 50));
  }
}

async function byte(seconds) {
  const end = Date.now() + seconds * 1000;
  while (pending.length === 0) {
    const left = end - Date.now();
    if (left <= 0) return -1;
    await arrived(Math.min(left, 50));
  }
  const value = pending[0];
  pending = pending.subarray(1);
  return value;
}

const type = (data) => fs.writeSync(fd, data);

// ---- XMODEM-CRC -------------------------------------------------------------

function crc16(data) {
  let crc = 0;
  for (const value of data) {
    crc ^= value << 8;
    for (let i = 0; i < 8; i++) crc = crc & 0x8000 ? ((crc << 1) ^ 0x1021) & 0xffff : (crc << 1) & 0xffff;
  }
  return crc;
}

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
    if (!child) fail(`${step}: only protosim can be reset from here`);
    child.kill(step === 'reset' ? 'SIGUSR1' : 'SIGUSR2');
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
