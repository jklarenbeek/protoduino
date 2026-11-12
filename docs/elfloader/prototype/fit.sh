#!/bin/bash
# fit.sh <mcu> [block-bytes] - fill one boot section: start from the smallest
# loader and add features in priority order for as long as the result still
# fits the block the BOOTSZ fuses reserve. Prints the feature set it chose.
# Without a block size it fills the section boot.conf.h assumes for the part:
# 2 KB up to 64 KB of flash, 4 KB above.
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"; cd "$HERE"; mkdir -p out
MCU="$1"
FLASHEND=$(echo "#include <avr/io.h>
FLASHEND" | avr-gcc -mmcu=$MCU -E -P - | tail -n 1 | tr -d ' ()')
BLOCK="${2:-$(( FLASHEND > 0xFFFF ? 4096 : 2048 ))}"
START=$(printf '0x%X' $(( FLASHEND + 1 - BLOCK )))

# name : flags, most valuable first
FEATURES=(
  "family-check:-DELF_CONF_ARCH_CHECK=1"
  "page-service:-DBOOT_CONF_PAGE_SERVICE=1"
  "load-thread:-DBOOT_CONF_LOAD_SERVICE=1 -DBOOT_CONF_PAGE_SERVICE=1"
  "image-crc:-DBOOT_CONF_IMAGE_CRC=1"
  "merge-segments:-DELF_CONF_COALESCE=1 -DELF_CONF_MAX_SEGMENTS=3"
  "quick-restart:-DBOOT_CONF_WINDOW_AFTER=7"
  "signature:-DELF_CONF_SIGNATURE=1"
  "eeprom:-DELF_CONF_EEPROM=1"
  "info:-DBOOT_CONF_INFO=1 -DBOOT_CONF_INFO_TEXT=MCUNAME"
  "verify-at-start:-DBOOT_CONF_VERIFY=1"
)
# every switch starts off; later -D of the same macro wins
BASE="-DELF_CONF_ARCH_CHECK=0 -DBOOT_CONF_IMAGE_CRC=0 -DELF_CONF_COALESCE=0 -DELF_CONF_MAX_SEGMENTS=2"

size_of() {   # prints the loader size, or nothing if it does not build or link
  local out
  local args=() a
  for a in "$@"; do [ "$a" = "-DBOOT_CONF_INFO_TEXT=MCUNAME" ] && a="-DBOOT_CONF_INFO_TEXT=\"$MCU\""; args+=("$a"); done
  out=$(./build.sh "fit-$MCU-$BLOCK" "$MCU" "$START" "${args[@]}" 2>/dev/null) || return
  echo "$out" | sed -n 's/.*text= *\([0-9]*\).*/\1/p'
}

chosen="$BASE"; names=""
sz=$(size_of $chosen)
if [ -z "$sz" ] || [ "$sz" -gt "$BLOCK" ]; then
  printf '%-11s %5d B block: the smallest loader%s does not fit\n' "$MCU" "$BLOCK" "${sz:+ ($sz B)}"; exit 0
fi
for f in "${FEATURES[@]}"; do
  n="${f%%:*}"; fl="${f#*:}"
  try=$(size_of $chosen $fl)
  if [ -n "$try" ] && [ "$try" -le "$BLOCK" ]; then chosen="$chosen $fl"; names="$names $n"; sz=$try; fi
done
printf '%-11s %5d B block at %-8s %5d B used, %4d B spare:%s\n' "$MCU" "$BLOCK" "$START" "$sz" $(( BLOCK - sz )) "${names:- (smallest loader only)}"
