/* host_test.c - run the device core natively against real avr-gcc output.
 * usage: host_test <file.elf> <arch> <flash_out.bin> [window_hi] [chunk] */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "elfld.h"

static uint8_t flash[1 << 18], eeprom[4096], sig[3];
static uint32_t flash_hi, flash_bytes, ee_bytes, sig_bytes;

uint8_t elfld_sink(uint8_t space, elfld_addr_t addr, uint8_t byte)
{
  if (space == ELFLD_SPACE_FLASH) {
    if (addr >= sizeof flash) return 1;
    flash[addr] = byte; flash_bytes++;
    if (addr + 1 > flash_hi) flash_hi = addr + 1;
  } else if (space == ELFLD_SPACE_EEPROM) {
    if (addr >= sizeof eeprom) return 1;
    eeprom[addr] = byte; ee_bytes++;
  } else if (space == ELFLD_SPACE_SIG) {
    if (addr < 3) sig[addr] = byte;
    sig_bytes++;
  }
  return 0;
}

int main(int argc, char **argv)
{
  if (argc < 4) return 2;
  FILE *f = fopen(argv[1], "rb");
  if (!f) { perror(argv[1]); return 2; }
  static uint8_t file[1 << 22];
  size_t n = fread(file, 1, sizeof file, f);
  fclose(f);
  uint32_t hi = argc > 4 ? strtoul(argv[4], 0, 0) : sizeof flash;
  unsigned chunk = argc > 5 ? atoi(argv[5]) : 0;

  memset(flash, 0xFF, sizeof flash);
  elfld_t c;
  elfld_begin(&c, getenv("LO") ? strtoul(getenv("LO"), 0, 0) : 0, hi);
  c.arch = (uint8_t)atoi(argv[2]);

  size_t pos = 0; uint8_t r = ELFLD_MORE; unsigned seed = 12345;
  while (pos < n && r == ELFLD_MORE) {
    unsigned k = chunk ? chunk : 1 + (seed = seed * 1103515245u + 12345u) % 128;
    if (k > n - pos) k = (unsigned)(n - pos);
    r = elfld_write(&c, file + pos, (uint8_t)k);
    pos += k;
  }
  printf("result=0x%02x file=%zu consumed<=%zu segments=%u entry=0x%x flash=%u eeprom=%u sig=%u",
         r, n, pos, (unsigned)c.nseg, (unsigned)c.entry, flash_bytes, ee_bytes, sig_bytes);
#if ELFLD_FEAT_RAM_EXTENT
  printf(" ram_end=0x%x", c.ram_end);
#endif
  if (sig_bytes) printf(" sig=%02x%02x%02x", sig[2], sig[1], sig[0]);
  printf("\n");
  f = fopen(argv[3], "wb"); fwrite(flash, 1, flash_hi, f); fclose(f);
  if (ee_bytes) { char nm[512]; snprintf(nm, sizeof nm, "%s.eep", argv[3]); f = fopen(nm, "wb"); fwrite(eeprom, 1, ee_bytes, f); fclose(f); }
  return r == ELFLD_DONE ? 0 : 1;
}
