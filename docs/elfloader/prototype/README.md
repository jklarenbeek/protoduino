# ELF loader prototype

Measurement prototype for `docs/elf-loader-design.pdf`. It is not library code and
is deliberately kept outside `src/`, which the Arduino build compiles in full.

| File | What it is |
|------|------------|
| `elfld.h`, `elfld.c`, `elfld_conf.h` | The streaming ELF32 core. Builds for AVR and for the host. |
| `elfsess.h` | One load session: XMODEM-CRC in, core, page cache out. All state in a caller-owned workspace, so the bootloader, its ROM service and a kernel can all use it. |
| `boot.c` | The bootloader: session at reset, image record, and the service table (page write, load session). |
| `build.sh` | `./build.sh <name> <mcu> <boot_start> [-D...]` builds `out/<name>.elf` and prints its size. |
| `fit.sh` | `./fit.sh <mcu> <block-bytes>` fills one boot section: adds features in priority order while they fit. |
| `host_test.c` | Runs the core natively on a real `.elf` and writes the flash image it produced. |
| `sim_harness.c` | Runs the loader, or a kernel on top of a bootloader, inside simavr; sends a file with XMODEM; compares flash with simavr's own ELF reader. |
| `kernel/` | Stand-in kernel (`kernel_demo.c`), a module (`count_mod.c`), the shared descriptor (`pd_module.h`) and `build_kernel.sh`. |
| `module/pd-module.ld` | Linker script for modules; rejects static RAM. |
| `svc_app.c`, `big.c` | Application that programs a page through the service table; a 106 KB image for the ATmega2560. |
| `probes/` | Programs behind the UART and serial findings, built against protoduino's own sources. |
| `simprobes/` | Programs behind the simavr and protosim findings. |
| `make_fixtures.sh` | Builds the files the tests send and the stand-in kernels. |
| `run_tests.sh`, `run_modules.sh` | The simulated scenarios T1 to T13 and M1 to M8. |
| `results-*.txt` | Recorded output: sizes, block filling, T tests, M tests, simulator probes. |

Requirements: `gcc-avr`, `avr-libc`, `binutils-avr`, `libelf-dev`, and protosim built
with its simavr submodule (`./scripts/setup-simavr.sh && make`), checked out beside
this repository.

The Optiboot image used by M2, M4 and M8 is not included. Copy
`optiboot/bootloaders/optiboot/optiboot_atmega328.hex` (version 8 or later) from
github.com/Optiboot/optiboot into `fixtures/`.

```sh
./make_fixtures.sh

S=../../../../protosim/libraries/simavr/simavr
gcc -O2 -o sim_harness sim_harness.c -I$S/sim -I$S/sim/avr \
    $S/obj-x86_64-linux-gnu/libsimavr.a -lelf -lm -lpthread

./run_tests.sh       # T1..T13: firmware loading, refusals, service table, erased flash
./run_modules.sh     # M1..M8: run-time module loading, also on Optiboot
./fit.sh atmega328p 2048

# core on the host
gcc -std=c99 -O2 -DELFLD_FEAT_EEPROM=1 -DELFLD_FEAT_SIGNATURE=1 -o host_test host_test.c elfld.c
./host_test fixtures/app-atmega328p.elf 5 out.bin        # 5 = core family of an ATmega328P
avr-objcopy -O binary -j .text -j .data fixtures/app-atmega328p.elf ref.bin && cmp out.bin ref.bin
```

Kernel build switches (`kernel/kernel_demo.c`):

| Switch | Values |
|--------|--------|
| `PD_MODULES` | `0` no module support, `1` start modules already in flash, `2` also load at run time |
| `PD_LOADER` | `1` ROM: the bootloader's session service loads the module, `2` the kernel carries the session itself |
| `PD_FLASH` | `1` page writes through the protoduino service table, `2` through Optiboot's `do_spm` |

Known limits: sizes are from avr-gcc 7.3.0; nothing has run on hardware; the ATtiny85
build compiles but has never executed; the kernel is a stand-in, not protoduino's
scheduler; in the tests the service table sits four bytes below the end of flash
(`SVC_BACK=4`) because simavr's ELF reader cannot load the last flash byte.

The document is rebuilt with `node ../render.mjs` (needs the `playwright` package and
the Inter and DejaVu Sans Mono fonts).
