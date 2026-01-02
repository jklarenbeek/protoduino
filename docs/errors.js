#!/usr/bin/env node
// errors.js
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
const ERR_FINALIZED = 0x255 // the lifecycle has completely ended
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

// Each domain has 4 sections and 12 per root
function ERR_IS_SECTION(err) {
  return err_op_depth(err) === 2;
}

// Each section has 14 leafs and thus 48 per root
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

function ERR_IS_BALANCED_ROOT(err) {
  // return ERR_IS_MOVEMENT(err);
  return err_op_center(err) === err_op_inverse(err);
}

function ERR_IS_BALANCED_SHADOW(err) {
  return ERR_IS_BALANCED(err) && ERR_IS_SHADOW(err);
}

function ERR_IS_BALANCED_EDGE(err) {
  return !ERR_IS_BALANCED_ROOT(err) && ERR_IS_BALANCED(err) && !ERR_IS_SHADOW(err);
}

function ERR_IS_UNBALANCED_ROOT(err) {
  return ERR_IS_ABSTRACT(err);
}

function ERR_IS_UNBALANCED_TWIN(err) {
  // !ERR_IS_ABSTRACT(err) && ERR_IS_TWIN(err)
  return !ERR_IS_ABSTRACT(err) && ERR_IS_UNBALANCED(err) && ERR_IS_TWIN(err);
}

function ERR_IS_UNBALANCED_EDGE(err) {
  const ones = err_op_one_count(err);
  return ones === 1 || ones === 7;
}

function ERR_IS_UNBALANCED_OTHER(err) {
  return ERR_IS_UNBALANCED(err) && !ERR_IS_TWIN(err) && !ERR_IS_UNBALANCED_EDGE(err);
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

function classifyByte(b) {
  b = toByte(b);
  const classes = [];
  const symmetry = [];

  // Balanced/unbalanced
  if (ERR_IS_BALANCED(b)) classes.push('BALANCED');
  if (ERR_IS_UNBALANCED(b)) classes.push('UNBALANCED');

  // The four main quadrants
  if (ERR_IS_ABSTRACT(b)) classes.push('ABSTRACT');
  if (ERR_IS_MOVEMENT(b)) classes.push('MOVING');

  // Twin/shadow/etc
  if (ERR_IS_TWIN(b)) classes.push('TWIN');
  if (ERR_IS_SHADOW(b)) classes.push('SHADOW');
  if (ERR_IS_MIRROR(b)) classes.push('MIRROR');

  // Root/domain/section/leaf
  if (ERR_IS_ROOT(b)) classes.push('ROOT');
  if (ERR_IS_DOMAIN(b)) classes.push('DOMAIN');
  if (ERR_IS_SECTION(b)) classes.push('SECTION');
  if (ERR_IS_LEAF(b)) classes.push('LEAF');

  // Specific balanced classes
  if (ERR_IS_BALANCED_ROOT(b)) symmetry.push('BALANCED_ROOT');
  if (ERR_IS_BALANCED_SHADOW(b)) symmetry.push('BALANCED_SHADOW');
  if (ERR_IS_BALANCED_EDGE(b)) symmetry.push('BALANCED_EDGE');

  // Unbalanced specifics
  if (ERR_IS_UNBALANCED_ROOT(b)) symmetry.push('UNBALANCED_ROOT');
  if (ERR_IS_UNBALANCED_TWIN(b)) symmetry.push('UNBALANCED_TWIN');
  if (ERR_IS_UNBALANCED_EDGE(b)) symmetry.push('UNBALANCED_EDGE');
  if (ERR_IS_UNBALANCED_OTHER(b)) symmetry.push('UNBALANCED_OTHER');

  // Reserved values (as defined in the C macro)
  if (ERR_IS_RESERVED(b)) classes.push('RESERVED');

  // Add relations to roots and neighbors (diagnostic)
  const center = err_op_center(b);
  const inverse = err_op_inverse(b);
  const reverse = err_op_reverse(b);
  const opposite = err_op_opposite(b);
  const root = err_op_root(b);
  const depth = err_op_depth(b);
  const ones = err_op_one_count(b);
  const entropy = err_op_entropy(b);
  const balance = ones / 8.0;

  // relation to its center/inverse/reverse/opposite (to itself)
  const rel_center = err_op_relation(b, center);
  const rel_inverse = err_op_relation(b, inverse);
  const rel_reverse = err_op_relation(b, reverse);
  const rel_opposite = err_op_relation(b, opposite);

  return {
    value: b,
    hex: '0x' + b.toString(16).padStart(2, '0'),
    bin: formatBin8(b),
    classes,
    symmetry,
    center,
    root,
    depth,
    inverse,
    reverse,
    opposite,
    ones,
    zeros: 8 - ones,
    entropy,
    balance,
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
    if (col === 'classes') {
      return escapeCSV(Array.isArray(obj.classes) ? obj.classes.join('|') : obj.classes);
    }
    if (col === 'symmetry') {
      return escapeCSV(Array.isArray(obj.symmetry) ? obj.symmetry.join('|') : obj.symmetry);
    }
    if (col === 'entropy' || col === 'balance') {
      return escapeCSV(Number(obj[col]).toFixed(8).replace(/\.?0+$/, ''));
    }
    return escapeCSV(obj[col]);
  }).join(',');
}

function main_hierarchical_csv() {
  const columns = [
    'value', 'hex', 'bin', 'classes', 'symmetry', 'center', 'root', 'depth',
    'inverse', 'reverse', 'opposite', 'ones', 'zeros', 'entropy', 'balance'
  ];

  // 1. Data Collection
  const allBytes = [];
  for (let i = 0; i < 256; i++) {
    allBytes.push(classifyByte(i));
  }

  // 2. Tree Construction (Adjacency List: Parent -> Children)
  const tree = {}; // Map<int, Array<Object>>
  const roots = []; // Array<Object>

  allBytes.forEach(b => {
    if (ERR_IS_ROOT(b.value)) {
      roots.push(b);
    } else {
      const parentVal = b.center;
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

    console.log(`// [QUADRANT ${rIndex + 1}/4] ROOT: ${rootNode.hex} (${rootNode.classes.join('|')})`);
    console.log(objectToCSVRow(rootNode, columns));

    // Level 1: Domains (Children of Root)
    const domains = tree[rootNode.value] || [];
    sortChildren(domains);

    domains.forEach(domainNode => {
      console.log(objectToCSVRow(domainNode, columns));

      // Level 2: Sections (Children of Domain)
      const sections = tree[domainNode.value] || [];
      sortChildren(sections);

      sections.forEach(sectionNode => {
        console.log(objectToCSVRow(sectionNode, columns));

        // Level 3: Leafs (Children of Section)
        const leafs = tree[sectionNode.value] || [];
        sortChildren(leafs);

        leafs.forEach(leafNode => {
          console.log(objectToCSVRow(leafNode, columns));
        });
      });
    });
  });
}

main_hierarchical_csv();
