#!/usr/bin/env node
// vt-compare.mjs - replay terminal output and compare the screens it draws.
//
//   node tools/vt-compare.mjs <rows> <cols> <out.txt> [<reference.txt>] [--show <marker>] [--budget <marker>=<bytes> ...]
//
// The output of a sketch is replayed on a small VT terminal: cursor moves,
// SGR attributes, CR, BS, LF and printable UTF-8. Markers such as <A> in the
// stream are snapshots: the screen as it stands there, and the bytes since
// the previous marker. With a reference, each screen has to be the same cell
// for cell, attribute included. A budget fails when a frame sends more.
// The exit status is 0 when everything holds.
import fs from 'node:fs';

const args = process.argv.slice(2);
const positional = [];
const budgets = {};
const show = [];
for (let i = 0; i < args.length; i++) {
  if (args[i] === '--show') show.push(args[++i]);
  else if (args[i] === '--budget') {
    const [k, v] = args[++i].split('=');
    budgets[k] = Number(v);
  } else positional.push(args[i]);
}
const [rows, cols, file, reference] = [Number(positional[0]), Number(positional[1]), positional[2], positional[3]];
if (!rows || !cols || !file) {
  console.error('usage: vt-compare.mjs <rows> <cols> <out.txt> [<reference.txt>] [--show <marker>] [--budget <marker>=<bytes>]');
  process.exit(2);
}

// the screens at each marker, and the bytes of each frame
function replay(path) {
  const bytes = fs.readFileSync(path);
  const text = bytes.toString('utf8');
  const blank = (attr) => Array.from({ length: rows }, () => Array.from({ length: cols }, () => [' ', attr]));
  let grid = blank('0');
  let y = 0, x = 0, attr = '0', last = 0;
  const screens = new Map();
  for (let i = 0; i < text.length;) {
    const marker = /^<([A-Z])>/.exec(text.slice(i, i + 3));
    if (marker) {
      screens.set(marker[1], { grid: grid.map((r) => r.map((c) => [...c])), bytes: Buffer.byteLength(text.slice(last, i)) });
      i += 3;
      last = i;
      continue;
    }
    const c = text[i];
    if (c === '\x1b') {
      const m = /^\x1b\[([0-9;?]*)([A-Za-z])/.exec(text.slice(i, i + 24));
      if (!m) { i += 2; continue; }
      i += m[0].length;
      const p = m[1];
      if (m[2] === 'H') {
        const [r, k] = (p.split(';').concat(['1', '1'])).slice(0, 2).map((v) => Number(v || 1));
        y = r - 1; x = k - 1;
      } else if (m[2] === 'm') attr = p || '0';
      else if (m[2] === 'J') grid = blank(attr);
      continue;
    }
    const cp = text.codePointAt(i);
    i += cp > 0xffff ? 2 : 1;
    if (c === '\r') { x = 0; continue; }
    if (c === '\b') { x = Math.max(0, x - 1); continue; }
    if (c === '\n') { y = Math.min(rows - 1, y + 1); continue; }
    if (y >= 0 && y < rows && x >= 0 && x < cols) grid[y][x] = [String.fromCodePoint(cp), attr];
    if (++x >= cols) { x = 0; y = Math.min(rows - 1, y + 1); }
  }
  return screens;
}

const same = (a, b) => a.every((row, r) => row.every((cell, k) => cell[0] === b[r][k][0] && cell[1] === b[r][k][1]));

const out = replay(file);
const ref = reference ? replay(reference) : null;
let failed = 0;
for (const [k, s] of out) {
  let line = `${k}  ${String(s.bytes).padStart(5)} bytes`;
  if (ref) {
    const r = ref.get(k);
    const ok = r && same(s.grid, r.grid);
    line += `   full redraw ${String(r ? r.bytes : '-').padStart(5)}   ${ok ? 'same screen' : 'DIFFERENT SCREEN'}`;
    if (!ok) failed++;
  }
  if (k in budgets) {
    const ok = s.bytes <= budgets[k];
    line += `   budget ${budgets[k]}: ${ok ? 'ok' : 'OVER'}`;
    if (!ok) failed++;
  }
  console.log(line);
}
for (const k of show) {
  if (!out.has(k)) continue;
  console.log(`-- screen ${k}`);
  for (const row of out.get(k).grid) console.log(row.map((c) => c[0]).join(''));
}
if (reference && ref.size !== out.size) { console.log('the two runs have different markers'); failed++; }
process.exit(failed ? 1 : 0);
