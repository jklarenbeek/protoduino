/* An application that programs a flash page through the bootloader, using the
 * flash driver of the overlay (FLASH_CONF_DRIVER = FLASH_DRIVER_BOOTROM).
 * Build with -DBOOTROM_CONF_TABLE=<address of the table> (FLASHEND-11 on hardware).
 * -DSVC_PAGE=<address> chooses the page; above 64 KB it is read back with ELPM. */
#include <protoduino.h>
#include <sys/flash.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#ifndef SVC_PAGE
#define SVC_PAGE 0x6000
#endif
#if (FLASHEND > 0xFFFFUL)
#define SVC_RD8(a) pgm_read_byte_far(a)
#else
#define SVC_RD8(a) pgm_read_byte(a)
#endif

static uint8_t buf[SPM_PAGESIZE];
volatile uint8_t outcome;                 /* 1 = page reads back, 2 = no service table, 3 = mismatch */

int main(void)
{
  if (!flash_is_writable()) { outcome = 2; for (;;) { } }
  for (uint16_t i = 0; i < SPM_PAGESIZE; i++) buf[i] = (uint8_t)(0xA0 + i);
  sei();
  flash_page_write(SVC_PAGE, buf);
  outcome = 1;
  for (uint16_t i = 0; i < SPM_PAGESIZE; i++) if (SVC_RD8(SVC_PAGE + i) != buf[i]) outcome = 3;
  DDRB = (SREG & 0x80) ? 0xFF : 0x0F;     /* interrupts restored? */
  for (;;) { }
}
