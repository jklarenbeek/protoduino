#!/usr/bin/env node
// module.mjs - build a loadable protoduino module for one kernel build.
//
//   node tools/module.mjs <kernel.elf> <module> <number> [-o <file.elf>] [--mcu <part>] [-D...] [-I...]
//
//   <kernel.elf>  the .elf file of the sketch that will run the module. It has
//                 to include <sys/module.h> with MODULE_CONF_SUPPORT set, or
//                 <mod/loader.h>. The Arduino IDE writes it with
//                 "Sketch > Export Compiled Binary".
//   <module>      the name of a process type of src/mod (upper, vcc, ...), or
//                 a .c file that defines one and ends with MODULE(name);
//   <number>      the flash slot, counted from 0: the number the shell
//                 shows for it in "mods"
//
// A module is linked against the functions of exactly one kernel build and
// for one slot: this tool reads both from the kernel file. Needs avr-gcc.
import { execFileSync } from 'node:child_process';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');

function fail(message) {
  console.error(`module.mjs: ${message}`);
  process.exit(1);
}

// ---- the parts of an ELF32 file this tool needs ---------------------------

function readElf(file) {
  if (!fs.existsSync(file)) fail(`no file ${file}`);
  const b = fs.readFileSync(file);
  if (b.readUInt32BE(0) !== 0x7f454c46 || b[4] !== 1) fail(`${file} is not an ELF32 file`);
  const shoff = b.readUInt32LE(32), shentsize = b.readUInt16LE(46), shnum = b.readUInt16LE(48);
  const shstrndx = b.readUInt16LE(50);
  const sections = [];
  for (let i = 0; i < shnum; i++) {
    const o = shoff + i * shentsize;
    sections.push({
      name: b.readUInt32LE(o), type: b.readUInt32LE(o + 4), addr: b.readUInt32LE(o + 12),
      offset: b.readUInt32LE(o + 16), size: b.readUInt32LE(o + 20), link: b.readUInt32LE(o + 24),
    });
  }
  const text = (table, at) => b.toString('latin1', table.offset + at, b.indexOf(0, table.offset + at));
  for (const s of sections) s.name = text(sections[shstrndx], s.name);
  const bytes = (s) => b.subarray(s.offset, s.offset + s.size);

  const symbols = [];
  const symtab = sections.find((s) => s.type === 2);
  if (!symtab) fail(`${file} has no symbol table: it was stripped`);
  for (let o = symtab.offset; o < symtab.offset + symtab.size; o += 16) {
    const info = b[o + 12];
    symbols.push({
      name: text(sections[symtab.link], b.readUInt32LE(o)), value: b.readUInt32LE(o + 4),
      type: info & 15, section: b.readUInt16LE(o + 14),
    });
  }
  return { sections, symbols, bytes };
}

function crc16(data) {
  let crc = 0;
  for (const byte of data) {
    crc ^= byte << 8;
    for (let i = 0; i < 8; i++) crc = crc & 0x8000 ? ((crc << 1) ^ 0x1021) & 0xffff : (crc << 1) & 0xffff;
  }
  return crc;
}

// ---- arguments ------------------------------------------------------------

const args = process.argv.slice(2);
const pass = [];            // -D and -I go to the compiler
let out = null, mcu = null;
const plain = [];
for (let i = 0; i < args.length; i++) {
  if (args[i] === '-o') out = args[++i];
  else if (args[i] === '--mcu') mcu = args[++i];
  else if (/^-[DI]/.test(args[i])) pass.push(args[i]);
  else plain.push(args[i]);
}
if (plain.length !== 3) {
  fail('usage: module.mjs <kernel.elf> <module> <number> [-o <file.elf>] [--mcu <part>] [-D...] [-I...]');
}
const [kernelFile, moduleArg, numberArg] = plain;

// ---- the kernel -------------------------------------------------------------

const kernel = readElf(kernelFile);
const textSection = kernel.sections.find((s) => s.name === '.text');
const dataSection = kernel.sections.find((s) => s.name === '.data');
if (!textSection) fail(`${kernelFile} has no .text section`);

// its image in flash, as module_init() reads it: code, then the initial data
const image = Buffer.concat([kernel.bytes(textSection), dataSection ? kernel.bytes(dataSection) : Buffer.alloc(0)]);
const kernelId = crc16(image);

const symbol = (name) => kernel.symbols.find((s) => s.name === name && s.section !== 0);
const layoutSymbol = symbol('module_layout');
if (!layoutSymbol) {
  fail(`${kernelFile} has no module support: the sketch has to include <sys/module.h> with MODULE_CONF_SUPPORT set, or <mod/loader.h>`);
}
const at = layoutSymbol.value - textSection.addr;
const [base, slotSize, slots, ramSize, fCpu] = [0, 4, 8, 12, 16].map((o) => image.readUInt32LE(at + o));

if (!mcu) {
  // avr-gcc notes the part in the file
  const note = kernel.sections.find((s) => s.name === '.note.gnu.avr.deviceinfo');
  const found = note && kernel.bytes(note).toString('latin1').match(/(at[a-z0-9]+)\0/);
  if (!found) fail(`${kernelFile} does not name its part: add --mcu <part>`);
  mcu = found[1];
}

const slot = Number(numberArg);
if (!Number.isInteger(slot) || slot < 0 || slot >= slots) fail(`slot ${numberArg}: the kernel has slots 0 to ${slots - 1}`);
const address = base + slot * slotSize;

// ---- what the kernel offers: every function and variable with a name of its own

const taken = new Map();
for (const s of kernel.symbols) {
  if ((s.type !== 1 && s.type !== 2) || s.section === 0 || s.section >= 0xff00) continue;   // objects and functions
  if (!/^[A-Za-z_][A-Za-z0-9_]*$/.test(s.name)) continue;                                   // no clones, no C++ names
  if (/^(__vector|__bad_interrupt|__ctors|__dtors|__init|__do_|__stack|__heap|__data|__bss|__eeprom|_etext|_edata|_end|__trampolines|main$|exit$|_exit$)/.test(s.name)) continue;
  taken.set(s.name, taken.has(s.name) ? null : s.value);                                    // two of a name: neither
}
const abi = [...taken].filter(([, value]) => value !== null)
  .map(([name, value]) => `PROVIDE(${name} = 0x${value.toString(16)});`).join('\n');

// ---- the module -------------------------------------------------------------

const work = fs.mkdtempSync(path.join(os.tmpdir(), 'protoduino-module-'));
let source = moduleArg, name = path.basename(moduleArg).replace(/\.[ch]$/, '');
if (!/\.c$/.test(moduleArg)) {
  // a process type of src/mod, or a header of your own
  const header = fs.existsSync(moduleArg) ? path.resolve(moduleArg) : path.join(root, 'src', 'mod', `${moduleArg}.h`);
  if (!fs.existsSync(header)) fail(`no module ${moduleArg}: neither a file nor src/mod/${moduleArg}.h`);
  source = path.join(work, `${name}.c`);
  fs.writeFileSync(source, `#include <protoduino.h>\n#include <sys/process.h>\n#include "${header}"\n#include <sys/module.h>\n\nMODULE(${name});\n`);
}
out = out ?? `${name}-slot${slot}.elf`;
fs.writeFileSync(path.join(work, 'kernel.ld'), abi + '\n');

const gcc = [
  '-std=gnu11', '-Os', `-mmcu=${mcu}`, `-DF_CPU=${fCpu}UL`,
  '-include', path.join(root, 'tools', 'core', 'core.h'),
  `-I${path.join(root, 'src')}`, `-I${path.join(root, 'src', 'sys')}`, ...pass,
  '-ffunction-sections', '-fdata-sections', '-mrelax', '-w',
  '-nostartfiles', '-nostdlib',
  `-Wl,-T,${path.join(root, 'ld', 'module.ld')}`, `-Wl,-T,${path.join(work, 'kernel.ld')}`,
  '-Wl,-e,module_descriptor', '-Wl,--defsym=__do_clear_bss=0', '-Wl,--defsym=__do_copy_data=0',
  `-Wl,--defsym=__module_kernel_id=0x${kernelId.toString(16)}`, `-Wl,--defsym=__module_base=0x${address.toString(16)}`,
  '-o', out, source, '-lgcc',
];
try {
  execFileSync('avr-gcc', gcc, { stdio: ['ignore', 'inherit', 'pipe'] });
} catch (error) {
  const text = String(error.stderr ?? error.message);
  const missing = [...new Set([...text.matchAll(/undefined reference to `([^']+)'/g)].map((m) => m[1]))];
  if (missing.length) {
    fail(`the kernel does not offer ${missing.join(', ')}.\n` +
      'A kernel only contains what somebody in it refers to: name the functions in MODULE_CONF_API of the sketch (see src/sys/module.h).');
  }
  if (text.includes('module has static variables')) {
    fail(`${name} has static variables. A module has no RAM of its own: its state belongs in its process struct.`);
  }
  fail(text.trim());
} finally {
  fs.rmSync(work, { recursive: true, force: true });
}

const size = readElf(out).sections.find((s) => s.name === '.text').size;
if (size > slotSize) {
  fs.rmSync(out);
  fail(`${name} takes ${size} bytes of flash, a slot of this kernel holds ${slotSize}`);
}
console.log(`${out}: ${name} for ${mcu}, kernel ${kernelId.toString(16).padStart(4, '0')}, slot ${slot} at 0x${address.toString(16)}: ` +
  `${size} of ${slotSize} bytes (its process struct may take ${ramSize} bytes of RAM)`);
