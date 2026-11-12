# ELF loader test bench

Builds, runs and measures the loader of `../overlay` for `docs/elf-loader-design.pdf`.
Nothing here is library code, and nothing under `src/` was changed.

| Path | What it is |
|------|------------|
| `../overlay/` | The loader as it would land in the library: `src/sys/elf.*` (stream), `src/sys/elf/load.*` (load thread), `src/sys/bootrom.h`, `src/cpu/avr/flash.*`, `src/sys/module.*`, `src/sys/process/loader.*`, `boot/` (bootloader), `ld/module.ld`. Compiled against the real `src/`. |
| `build.sh` | `./build.sh <name> <mcu> <boot_start> [-D...]` builds the bootloader into `out/<name>.elf` and prints its size. `default` as `<boot_start>` takes the standard boot section of the part: 2 KB up to 64 KB of flash, 4 KB above. |
| `fit.sh` | `./fit.sh <mcu> [block-bytes]` fills one boot section: adds options in priority order while they fit. Without a size it fills the standard section of the part. |
| `sizes.sh` | The size tables: bootloader per part and option, test kernel per variant, library objects. |
| `host_test.c` | Runs the stream natively on a real `.elf` and writes the flash image it produced. |
| `sim_harness.c` | Runs the bootloader, or a kernel on top of a bootloader, inside simavr; sends a file with XMODEM; compares flash with simavr's own ELF reader. `--ask-info` sends `?` first; `--damage <addr>` flips a bit of the installed image and resets; `--watch <s>` counts resets and image starts; `--then-reset <cause>` resets with a chosen cause; `--expect-jump` answers an image that jumps to the bootloader. |
| `kernel/kernel.c` | Test kernel: a sketch on the real protoduino scheduler with the serial device process, a heartbeat process and the loader process. `core.c`/`core.h` stand in for the Arduino core. |
| `kernel/count_module.c` | A loadable module: one process type. |
| `kernel/build_kernel.sh` | Builds a kernel variant and a module linked against exactly that build. `FAIR=1` builds the scheduler experiment of the document. `MCU=atmega2560` builds for that part, against a copy of the library in which `sys/clock.h` also accepts it (finding 17 of the document). |
| `svc_app.c`, `big.c` | Application that programs a page through the bootloader (`-DSVC_PAGE=<address>` chooses the page); a 106 KB image for the ATmega2560. |
| `wdt_app.c`, `jump_app.c`, `seg_app.c` | An image that the watchdog resets after 250 ms; a running image that jumps to the bootloader to be replaced; firmware with four or five flash segments. |
| `probes/` | Programs behind the library findings (`run_probes.sh`, `process_probe.c`) and the earlier UART and serial findings. |
| `simprobes/` | Programs behind the simavr and protosim findings. |
| `make_fixtures.sh` | Builds the files the tests send, and the test kernels with their modules. |
| `run_tests.sh`, `run_modules.sh` | The simulated scenarios T1 to T29 (ATmega328P, ATmega1284P, ATmega2560) and M1 to M12 (ATmega328P, ATmega2560). |
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

./run_tests.sh          # T1..T29: firmware loading, refusals, page service, erased flash, every option on the large parts, watchdog and reset cause, segment count
./run_modules.sh        # M1..M12: run-time module loading on the real scheduler, also on Optiboot and on the ATmega2560
./fit.sh atmega328p 2048
./fit.sh atmega2560      # its standard section: 4 KB
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

Bootloader build switch: `-DFLASH_CONF_SPM_ASM=0` selects the C version of the page
programmer (2,006 bytes on the ATmega328P against 1,952 with the assembly one); both pass T1.

Known limits: sizes are from avr-gcc 7.3.0; nothing has run on hardware; the ATtiny85
build compiles but has never executed; module loading was run on the ATmega328P and,
with the clock header widened in a copy of the library, on the ATmega2560; simavr's
ATmega2560 core has no signature row, so the signature check runs on the ATmega328P
and ATmega1284P only; simavr's ELF reader loses the last flash byte, which the harness
puts back so that the service table is tested where it really is; simavr records
neither a reset through the pin nor a brown-out, so the tests that depend on the reset
cause write it into `MCUSR` (`--then-reset`, `--set-mcusr`).

The document is rebuilt with `node ../render.mjs` (needs the `playwright` package and
the Inter and DejaVu Sans Mono fonts).
