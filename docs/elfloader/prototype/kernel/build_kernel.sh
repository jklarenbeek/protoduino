#!/bin/bash
# build_kernel.sh <name> [-D flags]
#   -> kernel-<name>.elf: the test kernel on the real protoduino scheduler,
#      and module-<name>.elf: count_module.c linked against exactly that kernel build.
set -eu
HERE="$(cd "$(dirname "$0")" && pwd)"; cd "$HERE"; N="$1"; shift
LIB="$HERE/../../../../src"; LD="$HERE/../../../../ld"
export LIBSYS="$LIB/sys"
CF="-std=gnu11 -Os -mmcu=${MCU:-atmega328p} -DF_CPU=16000000UL -include $HERE/core.h -I$LIB -I$LIB/sys
    -DPROCESS_CONF_EVENT_QUEUE_SIZE=8 -DMODULE_CONF_BASE=0x6000
    -ffunction-sections -fdata-sections -mrelax -Wall -Wno-implicit-fallthrough -Wno-unused-function -Wno-unknown-pragmas"
# FAIR=1: experiment for the document. process_run() of the library serves polls and then
# returns; this builds against a copy in which it also dispatches one event after the polls.
PROCESS_C=$LIB/sys/process.c
if [ "${FAIR:-0}" = 1 ]; then
  PROCESS_C=process-fair.c
  python3 - <<'PY'
import os
sys_dir=os.environ['LIBSYS']
s=open(sys_dir+'/process.c',encoding='utf-8-sig').read()
a=s.index('void process_run(void)'); b=s.index('}',a)
new='void process_run(void)\n{\n  /* experiment: polls first, then one event in the same call */\n  do_poll();\n  do_event();\n}'
s=s[:a]+new+s[b+1:]
s=s.replace('#include "process.h"','#include "'+sys_dir+'/process.h"').replace('#include "ipc.h"','#include "'+sys_dir+'/ipc.h"')
open('process-fair.c','w').write(s)
PY
fi
avr-gcc $CF "$@" -Wl,--gc-sections -o kernel-$N.elf \
  kernel.c core.c \
  $PROCESS_C $LIB/sys/ipc.c $LIB/sys/process/serial0.c $LIB/sys/serial/serial0.c $LIB/cpu/avr/uart0.c $LIB/lib/ringb8.c \
  $LIB/cpu/avr/flash.c $LIB/sys/elf.c $LIB/sys/elf/load.c \
  2> kernel-$N.log || { echo "kernel $N: BUILD FAILED"; grep -E "error|undefined" kernel-$N.log | head -n 12; exit 1; }

# identity of this build: CRC-16 of its flash image, as module_init() computes it
avr-objcopy -O binary -j .text -j .data kernel-$N.elf kernel-$N.bin
KID=$(python3 -c "
import sys
c=0
for b in open(sys.argv[1],'rb').read():
    c^=b<<8
    for _ in range(8): c=((c<<1)^0x1021)&0xFFFF if c&0x8000 else (c<<1)&0xFFFF
print('0x%04x'%c)" kernel-$N.bin)

# what the kernel offers to modules: every global function and variable of this build
avr-nm kernel-$N.elf | awk '$2 ~ /^[TDBRW]$/ && $3 !~ /^(__vector|__vectors|__bad_interrupt|__ctors|__dtors|__init|__do_|__stack|__heap|_exit|exit|main|__data|__bss|__eeprom|_etext|_edata|_end|__trampolines|__DATA_REGION|__FUSE|__TEXT_REGION|__LOCK|__SIGNATURE|__USER)/ {printf "%s = 0x%s;\n", $3, $1}' > kernel-$N-abi.ld

avr-gcc $CF "$@" -nostartfiles -nostdlib -Wl,-T,$LD/module.ld -Wl,-T,kernel-$N-abi.ld \
  -Wl,-e,module_descriptor -Wl,--defsym=__do_clear_bss=0 -Wl,--defsym=__do_copy_data=0 -Wl,--defsym=__module_kernel_id=$KID -Wl,--defsym=__module_base=${MODULE_BASE:-0x6000} \
  -o module-$N.elf count_module.c -lgcc 2> module-$N.log || { echo "module $N: BUILD FAILED"; grep -E "error|undefined|ASSERT" module-$N.log | head -n 12; exit 1; }
set -- $(avr-size kernel-$N.elf | tail -n 1)
M=$(avr-size module-$N.elf | tail -n 1 | awk '{print $1}')
printf 'kernel %-16s text=%5d data=%3d bss=%4d   id=%s   module: %d bytes of flash, file %d bytes\n' "$N" "$1" "$2" "$3" "$KID" "$M" "$(stat -c %s module-$N.elf)"
