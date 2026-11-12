// line.mjs - the serial line to a protoduino system: a port of this
// computer, or the pseudo terminal of a protosim that is started here.
// Uses stty to set up a port: Linux and macOS.
import { execFileSync, spawn } from 'node:child_process';
import fs from 'node:fs';

const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

// options: port, baud, sim ("<protosim> <firmware> [options]"), simLog (file for what protosim prints)
// Throws an Error with a plain message.
export async function openLine({ port, baud, sim, simLog }) {
  let child = null;

  if (sim) {
    // protosim makes a pseudo terminal for the line and links it here
    const link = '/tmp/simavr-uart0';
    fs.rmSync(link, { force: true });
    const [command, ...rest] = sim.split(/\s+/).filter(Boolean);
    const log = simLog ? fs.openSync(simLog, 'w') : 'ignore';
    child = spawn(command, rest, { stdio: ['ignore', log, log] });
    let failed = null;
    child.on('error', (error) => { failed = error; });
    for (let i = 0; i < 100 && !fs.existsSync(link) && !failed; i++) await sleep(50);
    if (failed) throw new Error(`cannot start ${command}: ${failed.message}`);
    if (!fs.existsSync(link)) { child.kill(); throw new Error('protosim did not open its serial line'); }
    await sleep(200);
    port = fs.realpathSync(link);
    baud = null; // a pseudo terminal has no rate
  }
  try {
    const flag = process.platform === 'darwin' ? '-f' : '-F';
    execFileSync('stty', [flag, port, ...(baud ? [String(baud)] : []), 'raw', '-echo'], { stdio: 'ignore' });
  } catch (error) {
    if (child) child.kill();
    throw new Error(`cannot set up ${port}: ${error.message}`);
  }
  const fd = fs.openSync(port, fs.constants.O_RDWR | fs.constants.O_NOCTTY);

  const line = {
    child,
    simulated: Boolean(sim),
    pending: Buffer.alloc(0), // received, not yet looked at
    waiting: null,

    write(data) { fs.writeSync(fd, typeof data === 'string' ? Buffer.from(data, 'latin1') : data); },

    // true when something arrived within ms
    arrived(ms) {
      return new Promise((resolve) => {
        const timer = setTimeout(() => { line.waiting = null; resolve(false); }, ms);
        line.waiting = () => { clearTimeout(timer); line.waiting = null; resolve(true); };
      });
    },

    // the next byte, or -1 when none arrives within the time
    async byte(seconds) {
      const end = Date.now() + seconds * 1000;
      while (line.pending.length === 0) {
        const left = end - Date.now();
        if (left <= 0) return -1;
        await line.arrived(Math.min(left, 50));
      }
      const value = line.pending[0];
      line.pending = line.pending.subarray(1);
      return value;
    },

    // everything received so far
    take() {
      const data = line.pending;
      line.pending = Buffer.alloc(0);
      return data;
    },

    signal(name) {
      if (!child) throw new Error('only protosim can be reset from here');
      child.kill(name);
    },

    close() {
      if (child) child.kill();
      child = null;
    },
  };

  fs.createReadStream(null, { fd, autoClose: false, highWaterMark: 256 })
    .on('data', (data) => {
      line.pending = Buffer.concat([line.pending, data]);
      if (line.waiting) line.waiting();
    })
    .on('error', () => {});

  return line;
}
