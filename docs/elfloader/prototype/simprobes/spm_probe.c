/* spm_probe.c - what does the simulator's self-programming model allow?
 * Linked as an ordinary application at address 0 (NOT in the boot section). */
#include <avr/io.h>
#include <avr/boot.h>
#include <avr/pgmspace.h>

#define PAGE 0x6000u

static void put(char c) { while (!(UCSR0A & _BV(UDRE0))) { } UDR0 = c; }
static void puts_(const char *s) { while (*s) put(*s++); }
static void hex(uint8_t v) { const char *d = "0123456789abcdef"; put(d[v >> 4]); put(d[v & 15]); }
static void kv(const char *k, uint8_t v) { puts_(k); put('='); hex(v); put(' '); }

static void fill(uint8_t pattern)
{
  for (uint16_t i = 0; i < SPM_PAGESIZE; i += 2)
    boot_page_fill(PAGE + i, (uint16_t)pattern | ((uint16_t)pattern << 8));
}

int main(void)
{
  uint8_t spmcsr_after_write, a, b;
  uint16_t busy = 0;

  UCSR0A = _BV(U2X0); UBRR0L = 16; UCSR0B = _BV(TXEN0);

  /* 1. erase + write 0xF0 from the application section */
  boot_page_erase(PAGE); boot_spm_busy_wait();
  fill(0xF0);
  boot_page_write(PAGE);
  spmcsr_after_write = SPMCSR;
  while (boot_spm_busy()) busy++;
  /* 2. read back WITHOUT re-enabling the read-while-write section */
  a = pgm_read_byte(PAGE);

  /* 3. write 0x3C over it WITHOUT erasing: real flash can only clear bits (0xF0 & 0x3C = 0x30) */
  fill(0x3C);
  boot_page_write(PAGE); boot_spm_busy_wait();
  b = pgm_read_byte(PAGE);

  kv("pc_is_application_section", 1);
  kv("spmcsr_right_after_page_write", spmcsr_after_write);
  kv("busy_polls_high", busy >> 8); kv("busy_polls_low", (uint8_t)busy);
  kv("read_back_without_rww_enable", a);
  kv("after_second_write_without_erase", b);
  puts_("\nDONE\n");
  for (;;) { }
}
