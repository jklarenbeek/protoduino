/* seg_app.c - a firmware file with more flash segments than code and initial data:
 * blocks placed at fixed addresses, as a kernel with modules programmed alongside it
 * or a table at a known place would have. Build with
 *   -Wl,--section-start=.block1=0x5000 -Wl,--section-start=.block2=0x5800
 * for four flash segments, and with -DFIVE and --section-start=.block3=0x6000 for five. */
#include <avr/io.h>
#include <avr/pgmspace.h>

const uint8_t block1[48] __attribute__((section(".block1"), used)) = { 'b', 'l', 'o', 'c', 'k', '1', 1, 2, 3, 4 };
const uint8_t block2[80] __attribute__((section(".block2"), used)) = { 'b', 'l', 'o', 'c', 'k', '2', 5, 6, 7, 8 };
#ifdef FIVE
const uint8_t block3[16] __attribute__((section(".block3"), used)) = { 'b', 'l', 'o', 'c', 'k', '3' };
#endif

static uint8_t counter = 7;                    /* initial data: the second segment */

int main(void)
{
  DDRB = counter + pgm_read_byte(block1) + pgm_read_byte(block2);
  for (;;) { counter++; }
}
