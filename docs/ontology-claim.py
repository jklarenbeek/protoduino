#!/usr/bin/env python3
"""
numbers-proof.py
Python 3 implementation of the byte classification & hierarchy system
Produces hierarchical CSV output describing 0..255 bytes
"""

import datetime
import math


# Reserved / special root values
ERR_ROOT_INIT    = 0x00  # 00000000
ERR_ROOT_RUN     = 0xFF  # 11111111
ERR_ROOT_BEFORE  = 0x55  # 01010101
ERR_ROOT_AFTER   = 0xAA  # 10101010

ERR_SUCCESS    = 0x00
ERR_YIELDING   = 0x01
ERR_EXITING    = 0x02
ERR_ENDING     = 0x03
ERR_FINALIZED  = 0xFF

RESERVED_ERRORS = {
    ERR_SUCCESS, ERR_YIELDING, ERR_EXITING,
    ERR_ENDING, ERR_FINALIZED
}


def to_byte(x: int) -> int:
    return x & 0xFF


def err_op_inverse(err: int) -> int:
    return to_byte(~err)


def err_op_reverse(err: int) -> int:
    err = to_byte(err)
    # swap nibbles
    err = ((err & 0xF0) >> 4) | ((err & 0x0F) << 4)
    # swap pairs
    err = ((err & 0xCC) >> 2) | ((err & 0x33) << 2)
    # swap adjacent bits
    err = ((err & 0xAA) >> 1) | ((err & 0x55) << 1)
    return to_byte(err)


def err_is_mirror(err: int) -> bool:
    return to_byte(err) == err_op_reverse(err)


def err_nibble_left(err: int) -> int:
    return (err >> 4) & 0x0F


def err_nibble_right(err: int) -> int:
    return err & 0x0F


def err_is_shadow(err: int) -> bool:
    return err_nibble_left(err) == ((~err_nibble_right(err)) & 0xF)


def err_is_twin(err: int) -> bool:
    return err_nibble_left(err) == err_nibble_right(err)


def err_op_opposite(err: int) -> int:
    return to_byte((err_nibble_right(err) << 4) | err_nibble_left(err))


def err_op_center(err: int) -> int:
    return to_byte(((err << 1) & 0xF0) | ((err >> 1) & 0x0F))


def err_is_abstract(err: int) -> bool:
    return err in (ERR_ROOT_INIT, ERR_ROOT_RUN)


def err_is_movement(err: int) -> bool:
    return err in (ERR_ROOT_BEFORE, ERR_ROOT_AFTER)


# ─── Hierarchy ──────────────────────────────────────────────────────────────

def err_is_root(err: int) -> bool:
    return err_is_abstract(err) or err_is_movement(err)


def err_is_domain(err: int) -> bool:
    return not err_is_root(err) and err_is_root(err_op_center(err))


def err_op_depth(err: int) -> int:
    v = to_byte(err)
    depth = 0
    while not err_is_root(v) and depth < 16:
        v = err_op_center(v)
        depth += 1
    return depth


def err_is_section(err: int) -> bool:
    return err_op_depth(err) == 2


def err_is_leaf(err: int) -> bool:
    return err_op_depth(err) == 3


# ─── Symmetry & other properties ────────────────────────────────────────────

def err_op_popcount(err: int) -> int:
    # Brian Kernighan way
    count = 0
    v = to_byte(err)
    while v:
        v &= v - 1
        count += 1
    return count


def err_is_balanced(err: int) -> bool:
    return err_op_popcount(err) == 4


def err_is_parity(err: int) -> bool:
    return err_op_center(err) == err_op_inverse(err)


def err_is_anticode(err: int) -> bool:
    return err_is_balanced(err) and err_is_shadow(err)


def err_is_margin(err: int) -> bool:
    return not err_is_parity(err) and err_is_balanced(err) and not err_is_shadow(err)


def err_is_repeater(err: int) -> bool:
    return (not err_is_abstract(err) and
            not err_is_balanced(err) and
            err_is_twin(err))


def err_is_impulse(err: int) -> bool:
    ones = err_op_popcount(err)
    return ones in (1, 7)


def err_is_asymmetry(err: int) -> bool:
    ones = err_op_popcount(err)
    return (not err_is_balanced(err) and
            not err_is_twin(err) and
            ones not in (1, 7))


def err_op_root(err: int) -> int:
    v = to_byte(err)
    for _ in range(16):
        if err_is_root(v):
            return v
        v = err_op_center(v)
    return v  # fallback (shouldn't happen)


def err_op_entropy(err: int) -> float:
    ones = err_op_popcount(err)
    zeros = 8 - ones
    if zeros == 0 or ones == 0:
        return 0.0
    p1 = ones / 8.0
    p0 = zeros / 8.0
    return -(p1 * math.log2(p1) + p0 * math.log2(p0))


# ─── Relations (mainly for documentation/debug) ─────────────────────────────

ERR_RELATION = {
    "DEFAULT": "DEFAULT",
    "CENTER": "CENTER",
    "OPPOSITE": "OPPOSITE",
    "REVERSED_TWINS": "REVERSED_TWINS",
    "REVERSED": "REVERSED",
    "INVERTED_TWINS": "INVERTED_TWINS",
    "INVERTED": "INVERTED",
}


def err_op_relation(left: int, right: int) -> str:
    left = to_byte(left)
    right = to_byte(right)

    if err_op_center(left) == right:
        return ERR_RELATION["CENTER"]
    if err_op_opposite(left) == right:
        return ERR_RELATION["OPPOSITE"]
    if err_op_reverse(left) == right:
        return ERR_RELATION["REVERSED_TWINS"] if err_is_twin(left) else ERR_RELATION["REVERSED"]
    if err_op_inverse(left) == right:
        return ERR_RELATION["INVERTED_TWINS"] if err_is_twin(left) else ERR_RELATION["INVERTED"]
    return ERR_RELATION["DEFAULT"]


# ─── Formatting helpers ─────────────────────────────────────────────────────

def format_bin8(x: int) -> str:
    return f"{x:08b}"


def to_hex8(value: int) -> str:
    return f"0x{value:02X}"


def escape_csv(val) -> str:
    s = str(val)
    if any(c in s for c in '",\n'):
        return f'"{s.replace('"', '""')}"'
    return s


# ─── Main classification ────────────────────────────────────────────────────

def classify_byte(b: int) -> dict:
    b = to_byte(b)

    classes = []
    hierarchy = []
    symmetry = []

    # Reserved
    if b in RESERVED_ERRORS:
        classes.append("RESERVED")

    root = err_op_root(b)
    if root == ERR_ROOT_INIT:   classes.append("INIT")
    if root == ERR_ROOT_RUN:    classes.append("RUN")
    if root == ERR_ROOT_BEFORE: classes.append("BEFORE")
    if root == ERR_ROOT_AFTER:  classes.append("AFTER")

    # Basic bit patterns
    if err_is_twin(b):    classes.append("TWIN")
    if err_is_shadow(b):  classes.append("SHADOW")
    if err_is_mirror(b):  classes.append("MIRROR")

    # Hierarchy level names
    if err_is_abstract(b):  hierarchy.append("ABSTRACT")
    if err_is_movement(b):  hierarchy.append("MOVING")

    if err_is_root(b):    hierarchy.append("ROOT")
    if err_is_domain(b):  hierarchy.append("DOMAIN")
    if err_is_section(b): hierarchy.append("SECTION")
    if err_is_leaf(b):    hierarchy.append("LEAF")

    # Balance classes
    if err_is_balanced(b):
        symmetry.append("BALANCED")
        if err_is_parity(b):    symmetry.append("PARITY")
        if err_is_anticode(b):  symmetry.append("ANTICODE")
        if err_is_margin(b):    symmetry.append("MARGIN")
    else:
        symmetry.append("UNBALANCED")
        if err_is_repeater(b):  symmetry.append("REPEATER")
        if err_is_impulse(b):   symmetry.append("IMPULSE")
        if err_is_asymmetry(b): symmetry.append("ASYMMETRY")

    return {
        "hex":      to_hex8(b),
        "classes":  "|".join(classes),
        "hierarchy": "|".join(hierarchy),
        "symmetry": "|".join(symmetry),
        "inverse":  to_hex8(err_op_inverse(b)),
        "opposite": to_hex8(err_op_opposite(b)),
        "reverse":  to_hex8(err_op_reverse(b)),
        "parent":   to_hex8(err_op_center(b)),
        "depth":    err_op_depth(b),
        "distance": err_op_popcount(b ^ root),
        "ones":     err_op_popcount(b),
        "entropy":  f"{err_op_entropy(b):.8f}".rstrip("0").rstrip("."),
        "bin":      format_bin8(b),
        "value":    b,
    }


def main_hierarchical_csv(print_leafs: bool = True):
    # Collect all classifications
    all_bytes = [classify_byte(i) for i in range(256)]

    columns = list(all_bytes[0].keys())

    # Build tree: parent_value → list of children dicts
    tree = {}
    roots = []

    for item in all_bytes:
        if err_is_root(item["value"]):
            roots.append(item)
        else:
            parent = err_op_center(item["value"])
            if parent not in tree:
                tree[parent] = []
            tree[parent].append(item)

    # Sort roots in natural order: 00 → 55 → AA → FF
    roots.sort(key=lambda x: x["value"])

    print("# " + "-"*77)
    print("# HYPER-BYTE TOPOLOGY EXPORT")
    print("# " + "-"*77)
    print(f"# Generated: {datetime.datetime.utcnow().isoformat(timespec='seconds')}Z")
    print("# Geometry:  16×16 Matrix (0..255) mapped to 4 Attractor Basins")
    print("# Hierarchy: ROOT(4) → DOMAIN(12) → SECTION(48) → LEAF(192)")
    print("# Logic:     parent = ((child << 1) & 0xF0) | ((child >> 1) & 0x0F)")
    print("# " + "-"*77)
    print(",".join(columns))

    for r_idx, root in enumerate(roots, 1):
        if r_idx > 1:
            print()

        print(f"# [QUADRANT {r_idx}/4] ROOT: {root['hex']} ({root['classes']})")
        print(",".join(escape_csv(root[col]) for col in columns))

        domains = sorted(tree.get(root["value"], []), key=lambda x: x["value"])

        for d_idx, domain in enumerate(domains, 1):
            print(f"# Domain {r_idx}.{d_idx}:")
            print(",".join(escape_csv(domain[col]) for col in columns))

            sections = sorted(tree.get(domain["value"], []), key=lambda x: x["value"])

            for s_idx, section in enumerate(sections, 1):
                print(f"# Section {r_idx}.{d_idx}.{s_idx}:")
                print(",".join(escape_csv(section[col]) for col in columns))

                if print_leafs:
                    leafs = sorted(tree.get(section["value"], []), key=lambda x: x["value"])
                    for leaf in leafs:
                        print(",".join(escape_csv(leaf[col]) for col in columns))


if __name__ == "__main__":
    main_hierarchical_csv(print_leafs=True)