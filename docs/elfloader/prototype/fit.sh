#!/bin/bash
# fit.sh <mcu> <block-bytes> - fill one boot section: start from the smallest
# loader and add features in priority order for as long as the result still
# fits the block the BOOTSZ fuses reserve. Prints the feature set it chose.
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"; cd "$HERE"; mkdir -p out
MCU="$1"; BLOCK="$2"
FLASHEND=$(echo "#include <avr/io.h>
FLASHEND" | avr-gcc -mmcu=$MCU -E -P - | tail -n 1 | tr -d ' ()')
START=$(printf '0x%X' $(( FLASHEND + 1 - BLOCK )))

# name : flags, most valuable first
FEATURES=(
  "family-check:-DELFLD_FEAT_ARCH_CHECK=1"
  "page-service:-DELFBOOT_FEAT_SERVICE=1"
  "module-loader:-DELFBOOT_FEAT_MODULES=1 -DELFBOOT_FEAT_SERVICE=1"
  "image-crc:-DELFBOOT_FEAT_IMAGE_CRC=1"
  "merge-segments:-DELFLD_FEAT_COALESCE=1 -DELFLD_CONF_MAX_SEGMENTS=3"
  "signature:-DELFLD_FEAT_SIGNATURE=1"
  "eeprom:-DELFLD_FEAT_EEPROM=1"
  "info:-DELFBOOT_FEAT_INFO=1 -DELFBOOT_INFO_STRING=MCUNAME"
  "verify-at-start:-DELFBOOT_FEAT_VERIFY_BOOT=1"
)
# every switch starts off; later -D of the same macro wins
BASE="-DELFLD_FEAT_ARCH_CHECK=0 -DELFBOOT_FEAT_IMAGE_CRC=0 -DELFLD_FEAT_COALESCE=0 -DELFLD_CONF_MAX_SEGMENTS=2"

size_of() {   # prints the loader size, or nothing if it does not build or link
  local out
  local args=() a
  for a in "$@"; do [ "$a" = "-DELFBOOT_INFO_STRING=MCUNAME" ] && a="-DELFBOOT_INFO_STRING=\"$MCU\""; args+=("$a"); done
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
