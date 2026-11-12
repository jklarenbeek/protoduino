#!/bin/bash
# run_probes.sh - the experiments behind the library findings of the document (ATmega328P, simavr).
HERE="$(cd "$(dirname "$0")" && pwd)"; cd "$HERE"; P=..; LIB=../../../../src; OVL=../../overlay; T=$P/out; mkdir -p $T
CF="-std=gnu11 -Os -mmcu=atmega328p -DF_CPU=16000000UL -include $P/kernel/core.h -I$LIB -I$LIB/sys -ffunction-sections -fdata-sections -Wl,--gc-sections -w"
SRC="$P/kernel/core.c $LIB/sys/process.c $LIB/sys/ipc.c $LIB/sys/process/serial0.c $LIB/sys/serial/serial0.c $LIB/cpu/avr/uart0.c $LIB/lib/ringb8.c"
(cd $P && ./build.sh "328P base" atmega328p 0x7800 >/dev/null)
probe() {  # <n> <ram bytes to show>
  avr-gcc $CF -DPROBE=$1 -o $T/probe$1.elf process_probe.c $SRC 2>/dev/null || return
  local a=$(avr-nm $T/probe$1.elf | awk '$3=="result"{print "0x"substr($1,5)}')
  $P/sim_harness atmega328p "$T/328P base.elf" $P/kernel/module-none.elf --kernel $T/probe$1.elf --no-send --dump-ram ${3:-$a} $2 2>&1 | grep -E "kernel output|ram @"
}
echo "--- 1. CC_EXTERN is empty in C: a variable declared with it is defined in every C file"
printf '#include <sys/process.h>\nint a(void){return process_current!=0;}\n' > $T/a.c
printf '#include <sys/process.h>\nint main(void){return process_current==0;}\n' > $T/b.c
avr-gcc $CF -c $T/a.c -o $T/a.o && avr-nm $T/a.o | grep process_current
echo "    two such files, linked with -fno-common (the default from GCC 10 on):"
avr-gcc $CF -fno-common $T/a.c $T/b.c -o $T/ab.elf 2>&1 | grep -E "multiple definition" | sed 's|/tmp/[A-Za-z0-9]*\.o|x.o|' | head -n 1
echo "--- 2. serial ring buffers on the ATmega328P (2048 bytes of RAM; serial.conf.h asks for 64 above 1023)"
avr-gcc $CF -c $LIB/sys/serial/serial0.c -o $T/serial0.o && avr-nm -S $T/serial0.o | grep ringb8_data
echo "--- 3. 80 bytes written to serial_process0 at once, nobody polls it again: bytes that left the UART"
probe 3 1 | sed 's/kernel output  *: /text: /' | awk '/text:/{t=$0; sub(/^text: /,"",t); print "    " length(t) " of 80 bytes: " t}'
echo "--- 4. baud = 115200 set before process_new(): UBRR0L (16 = 115200, 207 = 9600)"
probe 4 1 0xC4 | grep "ram @"
echo "--- 5. a process that ends: result[0] ran, result[1] PT_FINALLY ran, result[2] state (0 NONE, 4 EXITING), after 2.5 s"
probe 5 3 | grep "ram @"
echo "--- 6. a process polls itself every turn: result[0] its turns (mod 256), result[1] second process got INIT, after 2.5 s"
probe 6 2 | grep "ram @"
echo "--- 7. sys/clock.h on an ATmega2560"
printf '#include <sys/timer.h>\nint main(void){return 0;}\n' > $T/c.c
avr-gcc -std=gnu11 -mmcu=atmega2560 -DF_CPU=16000000UL -include $P/kernel/core.h -I$LIB -I$LIB/sys -c $T/c.c -o $T/c.o 2>&1 | grep -E "error" | sed "s|.*src/|src/|" | head -n 1
echo "--- 8. PT_PIPE_AVAILABLE() with the struct ipc_pt it is documented for"
printf '#include <sys/pt/pipe.h>\nint f(struct ipc_pt *pt){return PT_PIPE_AVAILABLE(pt);}\n' > $T/d.c
avr-gcc $CF -c $T/d.c -o $T/d.o 2>&1 | grep -E "error" | sed "s|.*d.c:[0-9:]* ||" | head -n 1
echo "--- 9. the receive callback serial_process0 installs"
avr-gcc -std=gnu11 -Os -mmcu=atmega328p -DF_CPU=16000000UL -include $P/kernel/core.h -I$LIB -I$LIB/sys -c $LIB/sys/process/serial0.c -o $T/sp0.o 2>&1 | grep -E "incompatible" | sed "s|.*src/|src/|" | head -n 1
echo "--- 10. uart0_openex() and the division it pulls in, bytes"
avr-gcc $CF -o $T/open.elf -x c - $LIB/cpu/avr/uart0.c 2>/dev/null <<< '#include <sys/uart.h>
int main(void){ uart0_open(115200); for(;;){} }' && avr-nm -S --size-sort $T/open.elf | grep -E "uart0_openex|__udivmodsi4"
