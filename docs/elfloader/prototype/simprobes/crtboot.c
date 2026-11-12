#include <avr/io.h>
static void put(char c) { while (!(UCSR0A & _BV(UDRE0))) { } UDR0 = c; }
int main(void) { UCSR0A = _BV(U2X0); UBRR0L = 16; UCSR0B = _BV(TXEN0); const char *s = "BOOT-BANNER\n"; while (*s) put(*s++); for (;;) { } }
