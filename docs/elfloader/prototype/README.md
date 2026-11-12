# ELF loader prototype

Measurement prototype for `docs/elf-loader-design.pdf`. It is not library code and
is deliberately kept outside `src/`, which the Arduino build compiles in full.

| File | What it is |
|------|------------|
| `elfld.h`, `elfld.c`, `elfld_conf.h` | The streaming ELF32 core. Builds for AVR and for the host. |
| `boot.c` | Serial bootloader around the core: XMODEM-CRC, page cache, image record, service entry. |
| `build.sh` | `./build.sh <name> <mcu> <boot_start> [-D...]` builds `out/<name>.elf` and prints its size. |
| `host_test.c` | Runs the core natively on a real `.elf` and writes the flash image it produced. |
| `sim_harness.c` | Runs the bootloader inside simavr, sends a file with XMODEM, compares flash with simavr's own ELF reader. `--fallthrough <addr> <r31>` starts execution in erased flash instead. |
| `svc_app.c` | Application that programs a page through the loader's service entry. |
| `big.c` | A 106 KB image for the ATmega2560 that reaches far flash. |
| `module/` | Link recipe for a prelinked module: `kernel.c`, `blink_mod.c`, `pd-module.ld`. |
| `probes/` | Programs that produced the UART and serial findings, built against protoduino's own sources. |
| `simprobes/` | Programs that produced the simavr and protosim findings (section 8 of the document). |
| `results-sizes.txt`, `results-tests.txt`, `results-simulators.txt` | Recorded output of the size matrix, the simulated scenarios T1 to T12, and the simulator probes including T13. |

Requirements: `gcc-avr`, `avr-libc`, `binutils-avr`, `libelf-dev`, and protosim built
with its simavr submodule (`./scripts/setup-simavr.sh && make`).

```sh
# core on the host
gcc -std=c99 -O2 -DELFLD_FEAT_EEPROM=1 -DELFLD_FEAT_SIGNATURE=1 -o host_test host_test.c elfld.c
./host_test app.elf 5 out.bin        # 5 = core family of an ATmega328P (avr:5)
avr-objcopy -O binary -j .text -j .data app.elf ref.bin && cmp out.bin ref.bin

# bootloader
./build.sh m328 atmega328p 0x7800
./build.sh m2560 atmega2560 0x3F000

# end to end in simavr (protosim checked out beside protoduino)
S=../../../../protosim/libraries/simavr/simavr
gcc -O2 -o sim_harness sim_harness.c -I$S/sim -I$S/sim/avr \
    $S/obj-x86_64-linux-gnu/libsimavr.a -lelf -lm -lpthread
./sim_harness atmega328p out/m328.elf app.elf --corrupt 3 --drop-ack 5

# module recipe
cd module
avr-gcc -Os -mmcu=atmega328p -o kernel.elf kernel.c
avr-nm kernel.elf | awk '$2=="T" && $3 ~ /^kernel_/ {printf "PROVIDE(%s = 0x%s);\n", $3, $1}' > kernel-abi.ld
avr-gcc -Os -mmcu=atmega328p -nostartfiles -nostdlib -Wl,-T,pd-module.ld -Wl,-T,kernel-abi.ld \
        -Wl,-e,pd_module -Wl,--defsym=__pd_kernel_id=0x8013 -o blink_mod.elf blink_mod.c

# simulator probes (from simprobes/): plain applications, run under protosim
avr-gcc -std=gnu11 -Os -mmcu=atmega328p -DF_CPU=16000000UL -o spm_probe.elf spm_probe.c
$SIM spm_probe.elf -m atmega328p --uart0-out out.txt --exit-on-uart DONE --max-steps 20000000

# erased flash and the landing pad (control: -DELFBOOT_FEAT_LANDING_PAD=0)
./sim_harness atmega328p out/m328.elf app.elf --fallthrough 0x6002 0x80

# UART and serial probes (from probes/, with protoduino's src as $PD and protosim's binary as $SIM)
avr-gcc -std=gnu11 -Os -mmcu=atmega328p -DF_CPU=16000000UL -DTEST=1 -I$PD -I$PD/sys \
        -o probe.elf uart_probe.c $PD/cpu/avr/uart0.c
$SIM probe.elf -m atmega328p --uart0-out uart.txt --exit-on-uart DONE --max-steps 60000000
```

Known limits of the prototype: sizes are from avr-gcc 7.3.0; nothing has run on
hardware; the ATtiny85 build compiles but has never executed; the service test
places its stub four bytes below the end of flash because simavr's ELF reader
cannot load the last flash byte.

The document itself is rebuilt with `node ../render.mjs` (needs the `playwright`
package and the Inter and DejaVu Sans Mono fonts).
