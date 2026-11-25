#!/bin/bash
# build_rp2.sh <rp2040|rp2350> <sketch.ino> [-D...]  -> tools/out/<sketch>-<chip>.elf
# Builds a sketch for the RP2040 or RP2350 with the Pico SDK alone, the way
# build_sketch.sh does for an AVR: the sketch as C++, tools/core-rp2 in place
# of an Arduino core, and protoduino as a library. As an Arduino IDE does,
# every file under src/ is compiled into the library: a file for another
# platform has to compile to nothing. Prints flash and RAM.
# Needs PICO_SDK_PATH (a checkout of the Pico SDK 2.x), cmake and
# arm-none-eabi-gcc. WARN=1 builds with -Wall -Wextra -Wcast-align and shows
# the warnings of protoduino.
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"; LIB="$(cd "$HERE/../src" && pwd)"; OUT="$HERE/out"; mkdir -p "$OUT"
CHIP="$1"; INO="$(cd "$(dirname "$2")" && pwd)/$(basename "$2")"; shift 2
[ -n "$PICO_SDK_PATH" ] || { echo "set PICO_SDK_PATH to a checkout of the Pico SDK"; exit 2; }
case $CHIP in
  rp2040) BOARD=pico;  PLATFORM=rp2040 ;;
  rp2350) BOARD=pico2; PLATFORM=rp2350-arm-s ;;
  *) echo "chip: rp2040 or rp2350"; exit 2 ;;
esac
NAME="$(basename "$INO" .ino)-$CHIP"
B="$OUT/rp2-build/$NAME"; mkdir -p "$B"
cp "$INO" "$B/sketch.cpp"
DEFS=""; for d in "$@"; do DEFS="$DEFS \"$d\""; done
WARN_FLAGS="-w"; [ -n "$WARN" ] && WARN_FLAGS="-Wall -Wextra -Wcast-align -Wno-unused-parameter"
cat > "$B/CMakeLists.txt" <<CMAKE
cmake_minimum_required(VERSION 3.13)
include(\$ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
project(sketch C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()
file(GLOB_RECURSE PROTODUINO_SOURCES CONFIGURE_DEPENDS "$LIB/*.c" "$LIB/*.cpp" "$LIB/*.S")
add_library(protoduino STATIC \${PROTODUINO_SOURCES})
target_include_directories(protoduino PUBLIC "$LIB" "$LIB/sys")
target_compile_options(protoduino PUBLIC -include "$HERE/core-rp2/core.h" $DEFS)
target_compile_options(protoduino PRIVATE $WARN_FLAGS)
target_link_libraries(protoduino PUBLIC pico_stdlib hardware_adc hardware_pwm hardware_flash hardware_sync pico_multicore)
add_executable($NAME sketch.cpp "$HERE/core-rp2/core.c")
target_compile_options($NAME PRIVATE -w -fpermissive)
target_link_libraries($NAME protoduino)
pico_enable_stdio_uart($NAME 0)
pico_enable_stdio_usb($NAME 0)
CMAKE
cmake -S "$B" -B "$B/build" -DPICO_BOARD=$BOARD -DPICO_PLATFORM=$PLATFORM > "$B/configure.log" 2>&1 || { tail -20 "$B/configure.log"; exit 1; }
make -C "$B/build" -j"$(nproc 2>/dev/null || echo 4)" > "$B/build.log" 2>&1 || { grep -E "error|Error" "$B/build.log" | head -20; exit 1; }
[ -n "$WARN" ] && grep -E "warning:" "$B/build.log" | grep "$LIB" | sort -u
cp "$B/build/$NAME.elf" "$OUT/$NAME.elf"
set -- $(arm-none-eabi-size "$OUT/$NAME.elf" | tail -n 1)
printf '%-28s flash %6d B   RAM %6d B\n' "$NAME" $(( $1 + $2 )) $(( $2 + $3 ))
