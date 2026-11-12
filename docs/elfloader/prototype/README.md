# ELF loader test bench

Builds, runs and measures the loader of `../overlay` for `docs/elf-loader-design.pdf`.
Nothing here is library code, and nothing under `src/` was changed.

| Path | What it is |
|------|------------|
| `../overlay/` | The loader as it would land in the library: `src/sys/elf.*` (stream), `src/sys/elf/load.*` (load thread), `src/sys/bootrom.h`, `src/cpu/avr/flash.*`, `src/sys/module.*`, `src/sys/process/loader.*`, `boot/` (bootloader), `ld/module.ld`. Compiled against the real `src/`. |
| `build.sh` | `./build.sh <name> <mcu> <boot_start> [-D...]` builds the bootloader into `out/<name>.elf` and prints its size. |
| `fit.sh` | `./fit.sh <mcu> <block-bytes>` fills one boot section: adds options in priority order while they fit. |
| `sizes.sh` | The size tables: bootloader per part and option, test kernel per variant, library objects. |
| `host_test.c` | Runs the stream natively on a real `.elf` and writes the flash image it produced. |
| `sim_harness.c` | Runs the bootloader, or a kernel on top of a bootloader, inside simavr; sends a file with XMODEM; compares flash with simavr's own ELF reader. |
| `kernel/kernel.c` | Test kernel: a sketch on the real protoduino scheduler with the serial device process, a heartbeat process and the loader process. `core.c`/`core.h` stand in for the Arduino core. |
| `kernel/count_module.c` | A loadable module: one process type. |
| `kernel/build_kernel.sh` | Builds a kernel variant and a module linked against exactly that build. `FAIR=1` builds the scheduler experiment of the document. |
| `svc_app.c`, `big.c` | Application that programs a page through the bootloader; a 106 KB image for the ATmega2560. |
| `probes/` | Programs behind the library findings (`run_probes.sh`, `process_probe.c`) and the earlier UART and serial findings. |
| `simprobes/` | Programs behind the simavr and protosim findings. |
| `make_fixtures.sh` | Builds the files the tests send, and the test kernels with their modules. |
| `run_tests.sh`, `run_modules.sh` | The simulated scenarios T1 to T13 and M1 to M9. |
| `results-*.txt` | Recorded output: sizes, block filling, T tests, M tests, library probes, simulator probes. |

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

./run_tests.sh          # T1..T13: firmware loading, refusals, page service, erased flash
./run_modules.sh        # M1..M9: run-time module loading on the real scheduler, also on Optiboot
./fit.sh atmega328p 2048
./sizes.sh
probes/run_probes.sh    # the library findings of section 7

# stream on the host
gcc -std=gnu11 -O2 -DELF_CONF_EEPROM=1 -DELF_CONF_SIGNATURE=1 -DELF_CONF_MAX_SEGMENTS=6 \
    -I../overlay/src -I../../../src -o host_test host_test.c ../overlay/src/sys/elf.c
./host_test fixtures/app-atmega328p.elf 5 out.bin        # 5 = core family of an ATmega328P
avr-objcopy -O binary -j .text -j .data fixtures/app-atmega328p.elf ref.bin && cmp out.bin ref.bin
```

Kernel build switches (see `kernel/build_kernel.sh` and `make_fixtures.sh`):

| Switch | Values |
|--------|--------|
| `MODULE_CONF_SUPPORT` | `0` no module support, `1` start modules already in flash, `2` also load at run time |
| `LOADER_CONF_THREAD` | `1` schedule the load thread inside the protoduino bootloader, `2` link a copy into the kernel |
| `FLASH_CONF_DRIVER` | `2` page writes through the protoduino service table, `3` through Optiboot's `do_spm` |

Known limits: sizes are from avr-gcc 7.3.0; nothing has run on hardware; the ATtiny85
build compiles but has never executed; module loading was run on the ATmega328P only;
in the tests the service table sits four bytes below the end of flash (`SVC_BACK=4`)
because simavr's ELF reader cannot load the last flash byte.

The document is rebuilt with `node ../render.mjs` (needs the `playwright` package and
the Inter and DejaVu Sans Mono fonts).
