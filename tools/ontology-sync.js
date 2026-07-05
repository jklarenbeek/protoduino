#!/usr/bin/env node
// file: ./tools/ontology-sync.js
// Single-source-of-truth synchronizer for the protoduino event/error ontology.
//
// The ontology math lives in docs/ontology-claim.js (the CLAIM).
// The taxonomy names live in src/sys/errors.h and src/sys/events.h.
// This tool closes the loop so the artifacts can never drift again:
//
//   1. VALIDATES the taxonomy headers against the claim:
//      - every value 0x00..0xFF is named in both namespaces
//      - strict duality: every ERR_* has an EVENT_* at its inverse and
//        vice versa (reserved kernel codes are deliberately shared)
//      - every symmetry annotation in a header comment (IMPULSE,
//        ASYMMETRY, MARGIN, ANTICODE, REPEATER, ABSOLUTE, PARITY and
//        trailing TWIN/SHADOW/MIRROR tags) matches the computed class
//      - every hierarchy tag ([Root]/[Domain]/[Section]/[Leaf] and
//        [Domain header]/[Section header]) matches the computed depth
//      - the structural invariants of the ontology itself
//        (4/12/48/192 hierarchy, 16 TWINS/SHADOWS/MIRRORS, 70 BALANCED,
//        involution of inverse, root preserved under inversion, ...)
//
//   2. GENERATES (deterministically, no timestamps):
//      - docs/ontology-proof.csv          (the PROOF)
//      - src/dbg/errors-strings.inc       (flash-resident string table)
//      - src/dbg/events-strings.inc       (flash-resident string table)
//
// Usage:
//   node tools/ontology-sync.js          # validate + write artifacts
//   node tools/ontology-sync.js --check  # validate + fail if artifacts
//                                        # on disk differ (for CI)

import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

import {
  err_op_inverse, err_op_center, err_op_root, err_op_depth,
  err_op_one_count, err_op_reverse,
  ERR_IS_RESERVED, ERR_IS_ROOT, ERR_IS_TWIN, ERR_IS_SHADOW, ERR_IS_MIRROR,
  ERR_IS_BALANCED, ERR_IS_PARITY, ERR_IS_ANTICODE, ERR_IS_MARGIN,
  ERR_IS_ABSOLUTE, ERR_IS_REPEATER, ERR_IS_IMPULSE, ERR_IS_ASYMMETRY,
  buildHierarchicalCsvLines, toHex8,
} from '../docs/ontology-claim.js';

const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const CHECK_MODE = process.argv.includes('--check');

const FILES = {
  errors: path.join(ROOT, 'src/sys/errors.h'),
  events: path.join(ROOT, 'src/sys/events.h'),
  csv: path.join(ROOT, 'docs/ontology-proof.csv'),
  errStrings: path.join(ROOT, 'src/dbg/errors-strings.inc'),
  evtStrings: path.join(ROOT, 'src/dbg/events-strings.inc'),
};

const RESERVED = new Set([0x00, 0x01, 0x02, 0x03, 0xFF]);
const SYMMETRY_KEYWORDS = new Set([
  'PARITY', 'ANTICODE', 'MARGIN', 'ABSOLUTE', 'REPEATER', 'IMPULSE', 'ASYMMETRY',
]);
const STRUCT_TAGS = new Set(['TWIN', 'SHADOW', 'MIRROR']);

let failures = 0;
function fail(msg) {
  failures++;
  console.error(`  [FAIL] ${msg}`);
}

// ---------------------------------------------------------------------------
// Header parsing
// ---------------------------------------------------------------------------

/**
 * Parse #define lines from a taxonomy header.
 * Handles clang-format wrapped defines (backslash continuations) and
 * macro-to-macro aliases (e.g. #define ERR_OK ERR_SUCCESS).
 * Returns array of { name, value, comment, line }.
 */
function parseHeader(filepath) {
  const raw = fs.readFileSync(filepath, 'utf8');
  // Join continuation lines but remember the original line number.
  const lines = raw.split(/\r?\n/);
  const joined = [];
  for (let i = 0; i < lines.length; i++) {
    let line = lines[i];
    const lineNo = i + 1;
    while (line.trimEnd().endsWith('\\') && i + 1 < lines.length) {
      line = line.trimEnd().slice(0, -1) + ' ' + lines[++i].trim();
    }
    joined.push({ text: line, lineNo });
  }

  const defs = [];
  const byName = new Map();
  const re = /^#define\s+(\w+)\s+(0x[0-9A-Fa-f]{1,2}|\w+)\s*(?:\/\/\s*(.*))?$/;
  for (const { text, lineNo } of joined) {
    const m = re.exec(text.trim());
    if (!m) continue;
    const [, name, rawValue, comment] = m;
    let value;
    if (/^0x[0-9A-Fa-f]+$/.test(rawValue)) {
      value = parseInt(rawValue, 16);
    } else if (byName.has(rawValue)) {
      value = byName.get(rawValue).value; // alias to earlier macro
    } else {
      continue; // alias to something outside this header (e.g. version int)
    }
    const def = { name, value, comment: (comment || '').trim(), line: lineNo };
    defs.push(def);
    if (!byName.has(name)) byName.set(name, def);
  }
  return defs;
}

// ---------------------------------------------------------------------------
// Classification helpers (mirror of the claim)
// ---------------------------------------------------------------------------

function primarySymmetry(v) {
  if (ERR_IS_BALANCED(v)) {
    if (ERR_IS_PARITY(v)) return 'PARITY';
    if (ERR_IS_ANTICODE(v)) return 'ANTICODE';
    return 'MARGIN';
  }
  if (ERR_IS_ABSOLUTE(v)) return 'ABSOLUTE';
  if (ERR_IS_REPEATER(v)) return 'REPEATER';
  if (ERR_IS_IMPULSE(v)) return 'IMPULSE';
  return 'ASYMMETRY';
}

function hierarchyName(v) {
  return ['ROOT', 'DOMAIN', 'SECTION', 'LEAF'][err_op_depth(v)];
}

/**
 * Extract the human description from a define comment:
 * strip the leading symmetry keyword, [bracket] tags, trailing
 * TWIN/SHADOW/MIRROR structure tags and trailing duality annotations
 * like "=> ~ERR_X(0xNN)", "[~ERR_X]" or "(=EVENT_X, ...)".
 */
function extractDescription(comment) {
  let s = comment;
  s = s.replace(/=>.*$/, '');            // "=> ~ERR_FINALIZED(0xFF)"
  s = s.replace(/\[~\w+\]/g, '');        // "[~ERR_ACCESS_DENIED]"
  s = s.replace(/\(=\w+[^)]*\)/g, '');   // "(=EVENT_TEXT_RECV, data=...)"
  s = s.replace(/\[[^\]]*\]/g, '');      // "[Leaf]", "[Section header]"
  const words = s.trim().split(/\s+/).filter(Boolean);
  if (words.length && SYMMETRY_KEYWORDS.has(words[0])) words.shift();
  // strip trailing structure tags: "MIRROR", "TWIN+MIRROR", "SHADOW", ...
  while (words.length) {
    const last = words[words.length - 1];
    const parts = last.split('+');
    if (parts.every(p => STRUCT_TAGS.has(p))) words.pop();
    else break;
  }
  return words.join(' ');
}

/** Collect annotation claims made by a comment, for validation. */
function extractClaims(comment) {
  const claims = { symmetry: null, hierarchy: null, structs: [] };
  const words = comment.split(/\s+/).filter(Boolean);
  if (words.length && SYMMETRY_KEYWORDS.has(words[0])) claims.symmetry = words[0];
  const tag = /\[(Root|Domain|Section|Leaf)(?:\s+header)?\]/.exec(comment);
  if (tag) claims.hierarchy = tag[1].toUpperCase();
  // standalone uppercase structure tags anywhere in the comment
  for (const w of words) {
    for (const p of w.split('+')) {
      if (STRUCT_TAGS.has(p)) claims.structs.push(p);
    }
  }
  return claims;
}

// ---------------------------------------------------------------------------
// Validation passes
// ---------------------------------------------------------------------------

function validateInvariants() {
  console.log('-- ontology invariants (claim self-check)');
  const count = pred => {
    let n = 0;
    for (let v = 0; v < 256; v++) if (pred(v)) n++;
    return n;
  };
  const expect = (what, actual, wanted) => {
    if (actual !== wanted) fail(`${what}: expected ${wanted}, got ${actual}`);
  };
  expect('ROOT count', count(v => err_op_depth(v) === 0), 4);
  expect('DOMAIN count', count(v => err_op_depth(v) === 1), 12);
  expect('SECTION count', count(v => err_op_depth(v) === 2), 48);
  expect('LEAF count', count(v => err_op_depth(v) === 3), 192);
  expect('TWIN count', count(ERR_IS_TWIN), 16);
  expect('SHADOW count', count(ERR_IS_SHADOW), 16);
  expect('MIRROR count', count(ERR_IS_MIRROR), 16);
  expect('BALANCED count', count(ERR_IS_BALANCED), 70);
  expect('ANTICODE count', count(ERR_IS_ANTICODE), 16);
  expect('ANTICODE==SHADOW', count(v => ERR_IS_ANTICODE(v) !== ERR_IS_SHADOW(v)), 0);
  let onesSum = 0;
  for (let v = 0; v < 256; v++) {
    onesSum += err_op_one_count(v);
    if (err_op_inverse(err_op_inverse(v)) !== v) fail(`inverse involution broken at ${toHex8(v)}`);
    // Inversion maps each quadrant to its dual quadrant (Q1<->Q4, Q2<->Q3):
    if (err_op_root(err_op_inverse(v)) !== err_op_inverse(err_op_root(v)))
      fail(`root(~x) != ~root(x) at ${toHex8(v)}`);
    if (err_op_reverse(err_op_reverse(v)) !== v) fail(`reverse involution broken at ${toHex8(v)}`);
    if (err_op_depth(v) > 3) fail(`depth > 3 at ${toHex8(v)}`);
    if (!ERR_IS_ROOT(v) && err_op_depth(err_op_center(v)) !== err_op_depth(v) - 1)
      fail(`center does not decrease depth at ${toHex8(v)}`);
  }
  expect('sum of ones over all codes', onesSum, 1024);
}

function validateHeader(label, defs) {
  console.log(`-- ${label}: annotations vs computed classes`);
  const file = label;
  for (const d of defs) {
    if (!d.comment) continue;
    const claims = extractClaims(d.comment);
    if (claims.symmetry && claims.symmetry !== primarySymmetry(d.value)) {
      fail(`${file}:${d.line} ${d.name}(${toHex8(d.value)}) annotated ${claims.symmetry}, computed ${primarySymmetry(d.value)}`);
    }
    if (claims.hierarchy && claims.hierarchy !== hierarchyName(d.value)) {
      fail(`${file}:${d.line} ${d.name}(${toHex8(d.value)}) tagged [${claims.hierarchy}], computed [${hierarchyName(d.value)}]`);
    }
    for (const s of claims.structs) {
      const holds = s === 'TWIN' ? ERR_IS_TWIN(d.value)
        : s === 'SHADOW' ? ERR_IS_SHADOW(d.value)
        : ERR_IS_MIRROR(d.value);
      if (!holds) fail(`${file}:${d.line} ${d.name}(${toHex8(d.value)}) claims ${s}, which does not hold`);
    }
  }
}

function validateCoverageAndDuality(errDefs, evtDefs) {
  console.log('-- coverage & duality');
  const errByValue = new Map(); // value -> first name
  const evtByValue = new Map();
  for (const d of errDefs) {
    if (d.name.startsWith('ERR_') && !errByValue.has(d.value)) errByValue.set(d.value, d);
  }
  for (const d of evtDefs) {
    if (d.name.startsWith('EVENT_') && !evtByValue.has(d.value)) evtByValue.set(d.value, d);
  }
  for (let v = 0; v < 256; v++) {
    if (!errByValue.has(v)) fail(`no ERR_* name for ${toHex8(v)}`);
    if (!evtByValue.has(v)) fail(`no EVENT_* name for ${toHex8(v)}`);
    const inv = err_op_inverse(v);
    if (!RESERVED.has(inv) && errByValue.has(v) && !evtByValue.has(inv)) {
      fail(`duality hole: ${errByValue.get(v).name}(${toHex8(v)}) has no EVENT_* at ${toHex8(inv)}`);
    }
  }
  return { errByValue, evtByValue };
}

// ---------------------------------------------------------------------------
// Artifact generation
// ---------------------------------------------------------------------------

function cEscape(s) {
  return s.replace(/\\/g, '\\\\').replace(/"/g, '\\"');
}

/**
 * Emit a switch-body include file mapping codes to flash-resident strings.
 * The including .c file defines S(tiny, verbose) to select the message set
 * and wrap it in PSTR() so the literal lives in flash, never SRAM.
 */
function buildStringTable(kind, prefix, defs, byValue) {
  const out = [];
  out.push(`/* AUTO-GENERATED by tools/ontology-sync.js -- DO NOT EDIT.`);
  out.push(` * Source of truth: src/sys/${kind}.h (names + comment descriptions).`);
  out.push(` * Regenerate: node tools/ontology-sync.js`);
  out.push(` *`);
  out.push(` * This file is a switch body. The including file must define`);
  out.push(` *   S(tiny, verbose)`);
  out.push(` * selecting one literal and wrapping it in PSTR() so every string`);
  out.push(` * is flash-resident (PROGMEM on AVR); nothing is placed in SRAM.`);
  out.push(` */`);
  // aliases per value, for the reader
  const aliases = new Map();
  for (const d of defs) {
    if (!d.name.startsWith(prefix)) continue;
    const first = byValue.get(d.value);
    if (first && first.name !== d.name) {
      if (!aliases.has(d.value)) aliases.set(d.value, []);
      aliases.get(d.value).push(d.name);
    }
  }
  const values = [...byValue.keys()].sort((a, b) => a - b);
  for (const v of values) {
    const d = byValue.get(v);
    const tiny = d.name.replace(new RegExp(`^${prefix}`), '');
    const verbose = extractDescription(d.comment) || tiny;
    const extra = aliases.has(v) ? ` /* also: ${aliases.get(v).join(', ')} */` : '';
    out.push(`case ${d.name}: return S("${cEscape(tiny)}", "${cEscape(verbose)}");${extra}`);
  }
  return out.join('\n') + '\n';
}

function writeOrCheck(filepath, content) {
  const rel = path.relative(ROOT, filepath);
  if (CHECK_MODE) {
    let disk = null;
    try { disk = fs.readFileSync(filepath, 'utf8'); } catch { /* missing */ }
    const normalize = s => s == null ? null : s.replace(/\r\n/g, '\n').replace(/\n+$/, '\n');
    if (normalize(disk) !== normalize(content)) {
      fail(`${rel} is stale or missing — run: node tools/ontology-sync.js`);
    } else {
      console.log(`  [OK  ] ${rel} up to date`);
    }
  } else {
    fs.writeFileSync(filepath, content, 'utf8');
    console.log(`  [GEN ] ${rel}`);
  }
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

console.log(`=== protoduino ontology sync ${CHECK_MODE ? '(check mode)' : '(write mode)'} ===`);

const errDefs = parseHeader(FILES.errors);
const evtDefs = parseHeader(FILES.events);
console.log(`parsed ${errDefs.length} defines from src/sys/errors.h, ${evtDefs.length} from src/sys/events.h`);

validateInvariants();
validateHeader('src/sys/errors.h', errDefs);
validateHeader('src/sys/events.h', evtDefs);
const { errByValue, evtByValue } = validateCoverageAndDuality(errDefs, evtDefs);

if (failures > 0) {
  console.error(`\n=== ${failures} validation failure(s) — artifacts NOT ${CHECK_MODE ? 'checked' : 'written'} ===`);
  process.exit(1);
}

console.log('-- artifacts');
writeOrCheck(FILES.csv, buildHierarchicalCsvLines(true).join('\n') + '\n');
writeOrCheck(FILES.errStrings, buildStringTable('errors', 'ERR_', errDefs, errByValue));
writeOrCheck(FILES.evtStrings, buildStringTable('events', 'EVENT_', evtDefs, evtByValue));

if (failures > 0) {
  console.error(`\n=== FAIL: ${failures} problem(s) ===`);
  process.exit(1);
}
console.log('\n=== PASS: ontology is consistent ===');
