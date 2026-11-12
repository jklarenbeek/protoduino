/* An application that programs a flash page through the loader's service entry. */
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
typedef void (*write_page_fn)(uint32_t addr, const uint8_t *data);
static uint8_t buf[SPM_PAGESIZE];
volatile uint8_t outcome;                 /* 1 = service found and page reads back, 2 = no service */
int main(void)
{
  if (pgm_read_byte(SVC_AT + 2) != 'p' || pgm_read_byte(SVC_AT + 3) != 'D') { outcome = 2; for (;;) { } }
  for (uint8_t i = 0; i < SPM_PAGESIZE; i++) buf[i] = (uint8_t)(0xA0 + i);
  sei();
  ((write_page_fn)(SVC_AT / 2))(0x6000UL, buf);
  outcome = 1;
  for (uint8_t i = 0; i < SPM_PAGESIZE; i++) if (pgm_read_byte(0x6000 + i) != buf[i]) outcome = 3;
  DDRB = (SREG & 0x80) ? 0xFF : 0x0F;     /* interrupts restored? */
  for (;;) { }
}
