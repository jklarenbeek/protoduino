/* core.c - stand-in for an Arduino core on the RP2040 and RP2350, for
 * builds with the Pico SDK alone, such as the simulator runs of tools/.
 * main() calls setup() once and loop() for ever; when the sketch has a
 * setup1() or a loop1(), core 1 runs them the same way, as in arduino-pico. */
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/sync.h"
#include "../core/random.h"

unsigned long micros(void) { return time_us_32(); }
unsigned long millis(void) { return (unsigned long)(time_us_64() / 1000); }
void delay(unsigned long ms) { sleep_ms(ms); }
void delayMicroseconds(unsigned int us) { busy_wait_us_32(us); }

extern void setup(void);
extern void loop(void);
extern void setup1(void) __attribute__((weak));
extern void loop1(void) __attribute__((weak));

static void core1_main(void)
{
  if (setup1)
    setup1();
  for (;;)
  {
    if (loop1)
      loop1();
    else
      __wfe();
  }
}

int main(void)
{
  if (setup1 || loop1)
    multicore_launch_core1(core1_main);
  setup();
  for (;;)
    loop();
}
