// Minimal stand-in for the Arduino core: main(), sei(), delay().
#include <avr/interrupt.h>
#include <util/delay.h>
void setup(void);
void loop(void);
void delay(unsigned long ms) { while (ms--) _delay_ms(1); }
int main(void) { sei(); setup(); for (;;) loop(); }
