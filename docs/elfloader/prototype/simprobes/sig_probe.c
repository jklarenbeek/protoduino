/* sig_probe.c - reading the device signature from software.
 * On the device the signature row is read with LPM, which uses Z only; RAMPZ belongs
 * to ELPM. What does the simulator return, and what when RAMPZ is not zero, as it is
 * in a bootloader above 64 KB? Build for atmega328p, atmega1284p and atmega2560. */
#include <avr/io.h>
#include <avr/boot.h>

static void put(char c) { while (!(UCSR0A & _BV(UDRE0))) { } UDR0 = c; }
static void puts_(const char *s) { while (*s) put(*s++); }
static void hex(uint8_t v) { const char *d = "0123456789abcdef"; put(d[v >> 4]); put(d[v & 15]); }

static void sig(const char *label)
{
  puts_(label);
  for (uint8_t a = 0; a < 6; a += 2) { put(' '); hex(boot_signature_byte_get(a)); }
  put('\n');
}

int main(void)
{
  UCSR0A = _BV(U2X0); UBRR0L = 16; UCSR0B = _BV(TXEN0);
#ifdef RAMPZ
  RAMPZ = 0; sig("signature read with RAMPZ=0:");
  RAMPZ = 1; sig("signature read with RAMPZ=1:");
  RAMPZ = 0;
#else
  sig("signature read:");
#endif
  puts_("DONE\n");
  for (;;) { }
}
