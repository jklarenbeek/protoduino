#include <protoduino.h>
#include <sys/process.h>
#include <sys/autostart.h>
#include <sys/timer.h>

// LED Process
PROCESS(led_process, "LED Blinker", 2);
PROCESS_THREAD(led_process, ev, data)
{
  static struct etimer timer;
  PT_BEGIN(&self->pt);
  pinMode(LED_BUILTIN, OUTPUT);
  etimer_set(&timer, CLOCK_SECOND / 2);
  while (1)
  {
    PT_WAIT_UNTIL(&self->pt, etimer_expired(&timer));
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    etimer_reset(&timer);
  }
  PT_END(&self->pt);
}

AUTOSTART_PROCESSES(&led_process);

void setup()
{
  process_init();
  autostart_start();
}

void loop()
{
  process_run();
  delay(1);
}