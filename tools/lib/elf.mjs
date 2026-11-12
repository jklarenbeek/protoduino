// elf.mjs - the parts of an ELF32 file the host tools need.
import fs from 'node:fs';

// Reads sections and symbols. Throws an Error with a plain message.
export function readElf(file) {
  if (!fs.existsSync(file)) throw new Error(`no file ${file}`);
  const b = fs.readFileSync(file);
  if (b.length < 52 || b.readUInt32BE(0) !== 0x7f454c46 || b[4] !== 1) throw new Error(`${file} is not an ELF32 file`);
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
  const bytes = (s) => (s.type === 8 ? Buffer.alloc(s.size) : b.subarray(s.offset, s.offset + s.size)); // 8: no bits in the file

  const symbols = [];
  const symtab = sections.find((s) => s.type === 2);
  if (!symtab) throw new Error(`${file} has no symbol table: it was stripped`);
  for (let o = symtab.offset; o < symtab.offset + symtab.size; o += 16) {
    const info = b[o + 12];
    symbols.push({
      name: text(sections[symtab.link], b.readUInt32LE(o)), value: b.readUInt32LE(o + 4), size: b.readUInt32LE(o + 8),
      type: info & 15, section: b.readUInt16LE(o + 14),
    });
  }
  const section = (name) => sections.find((s) => s.name === name);
  return { sections, symbols, bytes, section };
}

// An AVR program as the part holds it: flash is the code followed by the
// initial values of the variables.
export function avrImage(elf) {
  const text = elf.section('.text'), data = elf.section('.data');
  if (!text) throw new Error('no .text section');
  const flash = Buffer.concat([elf.bytes(text), data ? elf.bytes(data) : Buffer.alloc(0)]);
  return { flash, base: text.addr };
}

// CRC-16 as XMODEM and the kernel identity use it.
export function crc16(data) {
  let crc = 0;
  for (const byte of data) {
    crc ^= byte << 8;
    for (let i = 0; i < 8; i++) crc = crc & 0x8000 ? ((crc << 1) ^ 0x1021) & 0xffff : (crc << 1) & 0xffff;
  }
  return crc;
}
