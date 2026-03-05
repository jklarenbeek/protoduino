#!/usr/bin/env node
// file: ./docs/numbers-proof.js
// Node.js 22.17.0 (ESM) implementation of the provided C99 macros and ops.
// Produces CSV describing hierarchy and symmetry classes for bytes 0..255.

const ERR_ROOT_INIT = 0x00;  // 00000000
const ERR_ROOT_RUN = 0xFF;   // 11111111
const ERR_ROOT_BEFORE = 0x55; // 01010101 (85)
const ERR_ROOT_AFTER = 0xAA;  // 10101010 (170)

const ERR_SUCCESS = 0x00
const ERR_YIELDING = 0x01
const ERR_EXITING = 0x02
const ERR_ENDING = 0x03
const ERR_FINALIZED = 0xFF // the lifecycle has completely ended
function ERR_IS_RESERVED(err) {
  return err === ERR_SUCCESS
    || err === ERR_YIELDING
    || err === ERR_EXITING
    || err === ERR_ENDING
    || err === ERR_FINALIZED;
}

function toByte(x) {
  return x & 0xFF;
}

function err_op_inverse(err) {
  // bitwise NOT then mask to 8 bits
  return toByte(~err);
}

function err_op_reverse(err) {
  // bit reversal for 8 bits (SWAR-like)
  err = toByte(err);
  // swap nibbles
  err = toByte(((err & 0xF0) >>> 4) | ((err & 0x0F) << 4));
  // swap pairs
  err = toByte(((err & 0xCC) >>> 2) | ((err & 0x33) << 2));
  // swap adjacent bits
  err = toByte(((err & 0xAA) >>> 1) | ((err & 0x55) << 1));
  return err;
}

function ERR_IS_MIRROR(err) {
  return toByte(err) === err_op_reverse(err);
}

function ERR_NIBBLE_LEFT(err) {
  return (err >>> 4) & 0x0F;
}
function ERR_NIBBLE_RIGHT(err) {
  return err & 0x0F;
}

function ERR_IS_SHADOW(err) {
  return ERR_NIBBLE_LEFT(err) === ( (~ERR_NIBBLE_RIGHT(err)) & 0xF );
}

function ERR_IS_TWIN(err) {
  return ERR_NIBBLE_LEFT(err) === ERR_NIBBLE_RIGHT(err);
}

function err_op_opposite(err) {
  // swap nibbles: (DDDD EEEE)
  return toByte((ERR_NIBBLE_RIGHT(err) << 4) | ERR_NIBBLE_LEFT(err));
}

function err_op_center(err) {
  // ((err << 1) & 0xF0) | ((err >> 1) & 0x0F)
  return toByte(((err << 1) & 0xF0) | ((err >>> 1) & 0x0F));
}

function ERR_IS_ABSTRACT(err) {
  return err === ERR_ROOT_INIT || err === ERR_ROOT_RUN;
}
function ERR_IS_MOVEMENT(err) {
  return err === ERR_ROOT_BEFORE || err === ERR_ROOT_AFTER;
}

/**
 *  HIERARCHY FUNCTIONS
 *
 */

// There are 4 roots in total
function ERR_IS_ROOT(err) {
  return ERR_IS_ABSTRACT(err) || ERR_IS_MOVEMENT(err);
}

// Each root has 3 domains, thus 12 domains in total
function ERR_IS_DOMAIN(err) {
  return !ERR_IS_ROOT(err) && ERR_IS_ROOT(err_op_center(err));
}

// Each domain has 4 sections, thus 12 per root
function ERR_IS_SECTION(err) {
  return err_op_depth(err) === 2;
}

// Each section has 14 leafs and thus 48 per root, totaling in 192 leafs.
function ERR_IS_LEAF(err) {
  return err_op_depth(err) === 3;
}

/**
 * SYMMETRY CLASSES
 *
 */

function err_op_one_count(err) {
  // popcount for 8-bit
  err = toByte(err);
  // Brian Kernighan's method
  let count = 0;
  while (err) {
    err &= err - 1;
    count++;
  }
  return count;
}

// Symmetrical amount of ones and zeros
function ERR_IS_BALANCED(err) {
  return err_op_one_count(err) === 4;
}
// A-Symmetrical amount of ones and zero's
function ERR_IS_UNBALANCED(err) {
  return !ERR_IS_BALANCED(err);
}

function ERR_IS_PARITY(err) {
  // return ERR_IS_MOVEMENT(err);
  return err_op_center(err) === err_op_inverse(err);
}

function ERR_IS_ANTICODE(err) {
  return ERR_IS_BALANCED(err) && ERR_IS_SHADOW(err);
}

function ERR_IS_MARGIN(err) {
  return !ERR_IS_PARITY(err) && ERR_IS_BALANCED(err) && !ERR_IS_SHADOW(err);
}

function ERR_IS_ABSOLUTE(err) {
  return ERR_IS_ABSTRACT(err);
}

function ERR_IS_REPEATER(err) {
  // !ERR_IS_ABSTRACT(err) && ERR_IS_TWIN(err)
  return !ERR_IS_ABSTRACT(err) && ERR_IS_UNBALANCED(err) && ERR_IS_TWIN(err);
}

function ERR_IS_IMPULSE(err) {
  const ones = err_op_one_count(err);
  return ones === 1 || ones === 7;
}

function ERR_IS_ASYMMETRY(err) {
  return ERR_IS_UNBALANCED(err) && !ERR_IS_TWIN(err) && !ERR_IS_IMPULSE(err);
}

function err_op_root(err) {
  let v = toByte(err);
  // safety max loops (8 is safe for 8-bit)
  const maxIter = 16;
  let i = 0;
  while (!ERR_IS_ROOT(v) && i++ < maxIter) {
    v = err_op_center(v);
  }
  return v;
}

function err_op_depth(err) {
  let v = toByte(err);
  let d = 0;
  const maxIter = 16;
  while (!ERR_IS_ROOT(v) && d++ < maxIter) {
    v = err_op_center(v);
  }
  // if it was already root, d is 0; otherwise d is number of applied centers (capped)
  return d === 0 && ERR_IS_ROOT(toByte(err)) ? 0 : d;
}

function err_op_distance(left, right) {
  return err_op_one_count(left ^ right);
}

function err_op_entropy(err) {
  // Shannon binary entropy for 8 bits normalized over 8 lines.
  const ones = err_op_one_count(err);
  const zeros = 8 - ones;
  if (zeros === 0 || ones === 0) return 0.0;
  const p1 = ones / 8.0;
  const p0 = zeros / 8.0;
  // Math.log2 is available
  return -(p1 * Math.log2(p1) + p0 * Math.log2(p0));
}

// Relation enum (string labels)
const ERR_RELATION = {
  DEFAULT: 'DEFAULT',
  CENTER: 'CENTER',
  OPPOSITE: 'OPPOSITE',
  REVERSED_TWINS: 'REVERSED_TWINS',
  REVERSED: 'REVERSED',
  INVERTED_TWINS: 'INVERTED_TWINS',
  INVERTED: 'INVERTED'
};

function err_op_relation(left, right) {
  left = toByte(left);
  right = toByte(right);
  if (err_op_center(left) === right)
    return ERR_RELATION.CENTER;
  if (err_op_opposite(left) === right)
    return ERR_RELATION.OPPOSITE;

  if (err_op_reverse(left) === right) {
    if (ERR_IS_TWIN(left))
      return ERR_RELATION.REVERSED_TWINS;
    else
      return ERR_RELATION.REVERSED;
  }
  if (err_op_inverse(left) === right) {
    if (ERR_IS_TWIN(left))
      return ERR_RELATION.INVERTED_TWINS;
    else
      return ERR_RELATION.INVERTED;
  }
  return ERR_RELATION.DEFAULT;
}

function formatBin8(x) {
  return x.toString(2).padStart(8, '0');
}

function toHex8(value) {
 return '0x' + value.toString(16).padStart(2, '0').toUpperCase();
}

function toList(arr) {
  return Array.isArray(arr) ? arr.join('|') : arr;
}

function toFloat(nmbr) {
  return Number(nmbr).toFixed(8).replace(/\.?0+$/, '');
}

function classifyByte(b) {
  b = toByte(b);
  const classes = [];
  const hierarchy = [];
  const symmetry = [];

  // Reserved values (as defined in the C `pt.h` FSM macro's)
  if (ERR_IS_RESERVED(b)) classes.push('RESERVED');

  if (err_op_root(b) === ERR_ROOT_INIT) classes.push('INIT');
  if (err_op_root(b) === ERR_ROOT_RUN) classes.push('RUN');
  if (err_op_root(b) === ERR_ROOT_BEFORE) classes.push('BEFORE');
  if (err_op_root(b) === ERR_ROOT_AFTER) classes.push('AFTER');

  // Operators per quadrant Twin/shadow/etc
  if (ERR_IS_TWIN(b)) classes.push('TWIN');
  if (ERR_IS_SHADOW(b)) classes.push('SHADOW');
  if (ERR_IS_MIRROR(b)) classes.push('MIRROR');

  // The four main quadrants divided in 2 classes
  if (ERR_IS_ABSTRACT(b)) hierarchy.push('ABSTRACT');
  if (ERR_IS_MOVEMENT(b)) hierarchy.push('MOVING');

  // Root/domain/section/leaf
  if (ERR_IS_ROOT(b)) hierarchy.push('ROOT');
  if (ERR_IS_DOMAIN(b)) hierarchy.push('DOMAIN');
  if (ERR_IS_SECTION(b)) hierarchy.push('SECTION');
  if (ERR_IS_LEAF(b)) hierarchy.push('LEAF');

  // Balanced/unbalanced
  if (ERR_IS_BALANCED(b)) symmetry.push('BALANCED');
  if (ERR_IS_UNBALANCED(b)) symmetry.push('UNBALANCED');

  // Specific balanced symmetry classes (always BALANCED)
  if (ERR_IS_PARITY(b)) symmetry.push('PARITY');
  if (ERR_IS_ANTICODE(b)) symmetry.push('ANTICODE');
  if (ERR_IS_MARGIN(b)) symmetry.push('MARGIN');

  // Specific unbalanced symmetry classes (always UNBALANCED)
  if (ERR_IS_ABSOLUTE(b)) symmetry.push('ABSOLUTE');
  if (ERR_IS_REPEATER(b)) symmetry.push('REPEATER');
  if (ERR_IS_IMPULSE(b)) symmetry.push('IMPULSE');
  if (ERR_IS_ASYMMETRY(b)) symmetry.push('ASYMMETRY');

  return {
    hex: toHex8(b),
    classes: toList(classes),
    hierarchy: toList(hierarchy),
    symmetry: toList(symmetry),
    inverse: toHex8(err_op_inverse(b)),
    opposite: toHex8(err_op_opposite(b)),
    reverse: toHex8(err_op_reverse(b)),
    parent: toHex8(err_op_center(b)),
    depth: err_op_depth(b),
    distance: err_op_distance(b, err_op_root(b)),
    ones: err_op_one_count(b),
    entropy: toFloat(err_op_entropy(b)),
    bin: formatBin8(b),
    value: b,
  };
}

// ---------------------------------------------------------
// CSV FORMATTER
// ---------------------------------------------------------

function escapeCSV(val) {
  const str = String(val);
  if (/[,"\n]/.test(str)) {
    return `"${str.replace(/"/g, '""')}"`;
  }
  return str;
}

function objectToCSVRow(obj, columns) {
  return columns.map(col => {
    return escapeCSV(obj[col]);
  }).join(',');
}

function main_hierarchical_csv(outputleafs = false) {

  // 1. Data Collection
  const allBytes = [];
  for (let i = 0; i < 256; i++) {
    allBytes.push(classifyByte(i));
  }

  // and get its columns
  const columns = Object.getOwnPropertyNames(allBytes[0]);

  // 2. Tree Construction (Adjacency List: Parent -> Children)
  const tree = {}; // Map<int, Array<Object>>
  const roots = []; // Array<Object>

  allBytes.forEach(b => {
    if (ERR_IS_ROOT(b.value)) {
      roots.push(b);
    } else {
      const parentVal = err_op_center(b.value);
      if (!tree[parentVal]) tree[parentVal] = [];
      tree[parentVal].push(b);
    }
  });

  // Sort roots by value for consistent output (00, 55, AA, FF)
  roots.sort((a, b) => a.value - b.value);

  // Helper to sort children (by value)
  const sortChildren = (arr) => arr.sort((a, b) => a.value - b.value);

  // 3. Output Generation

  // Header documentation
  console.log(`// -----------------------------------------------------------------------------`);
  console.log(`// HYPER-BYTE TOPOLOGY EXPORT`);
  console.log(`// -----------------------------------------------------------------------------`);
  console.log(`// Generated: ${new Date().toISOString()}`);
  console.log(`// Geometry:  16x16 Matrix (0..255) mapped to 4 Attractor Basins.`);
  console.log(`// Hierarchy: ROOT(4) -> DOMAIN(12) -> SECTION(48) -> LEAF(192)`);
  console.log(`// Logic:     Parent = ((Child << 1) & 0xF0) | ((Child >>> 1) & 0x0F)`);
  console.log(`// -----------------------------------------------------------------------------`);
  console.log(columns.join(','));

  // Iterate Quadrants (Roots)
  roots.forEach((rootNode, rIndex) => {

    // Separator between quadrants (except before the first one)
    if (rIndex > 0) console.log("");

    console.log(`// [QUADRANT ${rIndex + 1}/4] ROOT: ${rootNode.hex} (${rootNode.classes})`);
    console.log(objectToCSVRow(rootNode, columns));

    // Level 1: Domains (Children of Root)
    const domains = tree[rootNode.value] || [];
    sortChildren(domains);

    domains.forEach((domainNode, dIndex) => {
      console.log(`// Domain ${rIndex + 1}.${dIndex + 1}:`);
      console.log(objectToCSVRow(domainNode, columns));

      // Level 2: Sections (Children of Domain)
      const sections = tree[domainNode.value] || [];
      sortChildren(sections);

      sections.forEach((sectionNode, sIndex) => {
        console.log(`// Section ${rIndex + 1}.${dIndex + 1}.${sIndex + 1}:`);
        console.log(objectToCSVRow(sectionNode, columns));

        // Level 3: Leafs (Children of Section, 192 leafs in total)
        if (outputleafs) {
          const leafs = tree[sectionNode.value] || [];
          sortChildren(leafs);

          leafs.forEach(leafNode => {
            console.log(objectToCSVRow(leafNode, columns));
          });
        }
      });
    });
  });
}

main_hierarchical_csv(true);
