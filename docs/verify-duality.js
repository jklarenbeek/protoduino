#!/usr/bin/env node
// file: ./docs/verify-duality.js  (CommonJS)
// Verifies every EVENT_* code is the bitwise inverse (~ERR_* & 0xFF)
// Usage: node docs/verify-duality.js (from protoduino root)

'use strict';
const fs = require('fs');
const path = require('path');

function parseDefines(filepath) {
    const src = fs.readFileSync(filepath, 'utf8');
    const map = {};
    // Handle single-line:  #define NAME 0xHH
    const re1 = /^#define\s+(\w+)\s+(0x[0-9A-Fa-f]{2})\b/gm;
    // Handle clang-format wrapped: #define NAME \\\n  0xHH
    const re2 = /^#define\s+(\w+)\s+\\\r?\n\s+(0x[0-9A-Fa-f]{2})\b/gm;
    let m;
    while ((m = re1.exec(src)) !== null) map[m[1]] = parseInt(m[2], 16);
    while ((m = re2.exec(src)) !== null) map[m[1]] = parseInt(m[2], 16);
    return map;
}

const hex2 = n => '0x' + n.toString(16).padStart(2, '0').toUpperCase();
const RESERVED = new Set([0x00, 0x01, 0x02, 0x03, 0xFF]);

const root = path.resolve(__dirname, '..');
const errs = parseDefines(path.join(root, 'src/sys/errors.h'));
const evts = parseDefines(path.join(root, 'src/sys/events.h'));

let ok = 0, warn = 0, fail = 0;

console.log('=== Event/Error Duality Verification ===\n');

/* --- 1. Every ERR_* must have an EVENT_* at its inverse position --- */
const errEntries = Object.entries(errs)
    .filter(([k]) => k.startsWith('ERR_'))
    .sort((a, b) => a[1] - b[1]);

for (const [eName, eCode] of errEntries) {
    const inv = (~eCode) & 0xFF;
    const matches = Object.entries(evts).filter(([, v]) => v === inv);

    if (RESERVED.has(inv)) {
        // reserved codes are deliberately shared — always fine
        const resName = matches.length ? matches.map(([k]) => k).join('/') : 'RESERVED';
        console.log(`  [RESERVED] ${eName}(${hex2(eCode)}) <-> ${resName}(${hex2(inv)})`);
    } else if (matches.length === 0) {
        console.error(`  [MISSING ] ${eName}(${hex2(eCode)}) inverse ${hex2(inv)} has no EVENT_* !`);
        fail++;
    } else {
        const names = matches.map(([k]) => k).join(' / ');
        console.log(`  [OK      ] ${eName}(${hex2(eCode)}) <-> ${names}(${hex2(inv)})`);
        ok++;
    }
}

/* --- 2. Every non-reserved EVENT_* must have an ERR_* at its inverse --- */
console.log('\n--- Orphan EVENT_* check ---');
const evEntries = Object.entries(evts)
    .filter(([k]) => k.startsWith('EVENT_'))
    .sort((a, b) => a[1] - b[1]);

for (const [evName, evCode] of evEntries) {
    if (RESERVED.has(evCode)) continue;
    const inv = (~evCode) & 0xFF;
    const hits = Object.entries(errs).filter(([, v]) => v === inv);
    if (hits.length === 0) {
        console.warn(`  [WARN    ] ${evName}(${hex2(evCode)}) has no ERR_* at ${hex2(inv)}`);
        warn++;
    }
}

/* --- 3. Summary --- */
console.log(`\n=== ${ok} OK  ${warn} WARN  ${fail} FAIL ===`);
if (fail > 0) { console.error('FAIL: duality constraints violated!'); process.exit(1); }
else { console.log('PASS: all duality constraints satisfied.'); }
