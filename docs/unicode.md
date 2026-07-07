# Unicode & UTF-8 Support

Protoduino's text layer (`src/lib/text/`) is built around one principle:
**UTF-8 is the transport and storage format everywhere; runes are ephemeral
code-point values** that exist only for the duration of a classification or
case-conversion call. Strings travel as raw UTF-8 bytes (zero-copy through
mcurses, the TUI, and the pipes); decoding happens incrementally and only
where a decision needs a code-point.

| Module | Role |
|---|---|
| [`utf8.h`](../src/lib/text/utf8.h) / `utf8.c` | UTF-8 codec (16-bit and 32-bit) and string operations |
| [`utf8_iter.h`](../src/lib/text/utf8_iter.h) / [`utf8_iter32.h`](../src/lib/text/utf8_iter32.h) | Zero-allocation cursor/builder iteration |
| [`utf8_stream.h`](../src/lib/text/utf8_stream.h) | Arduino `Stream` codec |
| [`rune16.h`](../src/lib/text/rune16.h) / `rune16.c` | BMP classification & case tables (Plan 9 derived) |
| [`rune32.h`](../src/lib/text/rune32.h) / `rune32.c` | Full-Unicode code-points, emoji classification |

## rune16 vs rune32 — which one?

- **`rune16_t` is UCS-2, not UTF-16.** It is an *unsigned* 16-bit code-point
  covering the Basic Multilingual Plane (U+0000–U+FFFF). There is no
  surrogate-pair handling, so emoji and other supplementary-plane characters
  **cannot** be represented. Surrogate values (U+D800–U+DFFF) are rejected by
  the codec.
- **`rune32_t`** covers all of Unicode (U+0000–U+10FFFF) including emoji, at
  the cost of 32-bit arithmetic on an 8-bit MCU.

Rule of thumb: classify/case-convert through `rune16_*` when the text is
BMP-only; reach for `rune32_*` / the `utf8_*32` functions when emoji may
appear. Both APIs are code-point *services* — avoid storing strings as rune
arrays (it roughly doubles RAM for mostly-ASCII text); keep strings in UTF-8.

## Codec strictness

`utf8_torune16` / `utf8_torune32` / `utf8_getr` reject, returning the decode
error sentinel while still consuming the offending sequence:

- **Overlong encodings** (`C0 80`, `E0 80 80`, `F0 8F BF BF`, …)
- **Surrogates** (U+D800–U+DFFF encoded directly)
- **Out-of-range** values (above U+10FFFF)

`utf8_valid()` therefore accepts exactly the set of well-formed UTF-8
strings. `utf8_fromrune16` / `utf8_fromrune32` refuse to *produce* surrogates
or the error sentinel (they return 0).

## Flash layout & the stride-2 tables

All classification/case tables live in flash (`CC_PROGMEM`) and are read with
`pgm_read_word()` on AVR — SRAM cost is zero. Case mappings use three table
forms, binary-searched in order:

1. **Contiguous ranges** `(lo, hi, excess-500)` — e.g. A–Z.
2. **Stride-2 ranges** `(lo, hi, excess-500)` matching `c` in `[lo, hi]` with
   `(c - lo)` even — these compress the long alternating upper/lower singlet
   runs of Latin Extended, Cyrillic, Greek and Latin Extended Additional
   (Ā/ā, Ѡ/ѡ, Ḁ/ḁ, …), saving ~2 KB of flash versus one entry per code-point.
3. **Residual singlets** `(c, excess-500)` for everything irregular.

The stride tables were generated from the original Plan 9 singlet tables and
verified **exhaustively**: for every code-point 0x0000–0xFFFF, all eight
classification/case functions agree with the original table layout
(524,288 comparisons, 0 mismatches), and every valid BMP scalar round-trips
through the UTF-8 codec.

## PROTODUINO_UNICODE_LEVEL

Compile-time coverage knob (see [`rune16.h`](../src/lib/text/rune16.h)):

| Level | Coverage | Flash cost |
|---|---|---|
| `0` | ASCII only: A–Z/a–z case, ASCII whitespace. No tables. | ~3.2 KB smaller |
| `2` *(default)* | Full BMP tables | baseline |

Encoding, decoding and *printing* any Unicode text is unaffected by the
level — it only gates classification and case conversion. Set it for the
whole build so the library and sketch agree, e.g.:

```bash
arduino-cli compile --fqbn arduino:avr:uno --library . \
  --build-property "build.extra_flags=-DPROTODUINO_UNICODE_LEVEL=0" \
  examples/20-process-basic
```

## Conformance testing

[`examples/26-rune-conformance`](../examples/26-rune-conformance/26-rune-conformance.ino)
runs 61 checks **on the AVR itself** under protosim — this matters because
the PROGMEM table access cannot be exercised by a host build:

```bash
arduino-cli compile --fqbn arduino:avr:uno --library . \
    --build-path build/26 examples/26-rune-conformance
protosim build/26/26-rune-conformance.ino.elf -m atmega328p -f 16000000 \
    --uart0-out out.txt --exit-on-uart "<DONE>" --max-steps 80000000
```

Expected output ends with `TOTALS pass=61 fail=0`.

## History (July 2026 overhaul)

The layer was repaired and shrunk in one pass; if you are diffing old code,
the behavioural changes are:

- `rune16_t` changed from signed `int_least16_t` to **`uint16_t`** — the
  signed type broke the sorted-table binary search and UTF-8 encoding for
  every code-point above U+7FFF (Hangul, CJK-compat, fullwidth forms).
- `rune16.c` now reads its tables with `pgm_read_word()`; previously every
  classification/case call returned garbage **on AVR hardware** (host builds
  masked the bug). Caught by running example 26 under protosim: 39/61 checks
  failed before the fix.
- `rune32_toupper`/`rune32_tolower` and `utf8_toupper`/`utf8_tolower` were
  **inverted**: they "compensated" for a rune16 name swap that never existed.
  `utf8_toupper("héllo")` now correctly yields `"HÉLLO"`.
- Overlong/surrogate rejection added to all decoders (previously
  `utf8_valid("\xC0\x80")` returned true).
- Case singlet tables compressed to stride-2 ranges (~2 KB flash), and the
  `PROTODUINO_UNICODE_LEVEL` tier added.
