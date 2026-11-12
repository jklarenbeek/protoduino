#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <avr/fuse.h>
#include <avr/signature.h>
#include <avr/interrupt.h>
FUSES = { .low = LFUSE_DEFAULT, .high = HFUSE_DEFAULT, .extended = EFUSE_DEFAULT };
uint8_t EEMEM ee_cfg[5] = { 1, 2, 3, 4, 5 };
static const char banner[] PROGMEM = "hello from flash";
volatile uint16_t counter = 0x1234;          /* .data: initialised RAM */
volatile uint8_t  scratch[40];               /* .bss  */
ISR(TIMER0_OVF_vect) { counter++; }
int main(void) {
  scratch[0] = pgm_read_byte(&banner[0]);
  scratch[1] = eeprom_read_byte(&ee_cfg[0]);
  DDRB |= 1;
  for (;;) { PORTB ^= 1; counter += scratch[0]; }
}
