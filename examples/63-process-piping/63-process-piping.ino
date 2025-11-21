#include <protoduino.h>
#include <sys/process.h>
#include <sys/autostart.h>
#include <sys/timer.h>

// Generator Process
PROCESS(generator_process, "Data Generator", 1);
PROCESS_THREAD(generator_process, ev, data)
{
  static struct etimer timer;
  static int count = 0;
  PT_BEGIN(&self->pt);
  etimer_set(&timer, CLOCK_SECOND);
  while (1)
  {
    PT_WAIT_UNTIL(&self->pt, etimer_expired(&timer));
    char buf[20];
    sprintf(buf, "Data %d", count++);
    process_pipe_send(&self, (process_data_t)buf);
    etimer_reset(&timer);
  }
  PT_END(&self->pt);
}

// Consumer Process
PROCESS(consumer_process, "Data Consumer", 2);
PROCESS_THREAD(consumer_process, ev, data)
{
  PT_BEGIN(&self->pt);
  while (1)
  {
    PT_WAIT_UNTIL(&self->pt, ev == PROCESS_EVENT_PIPE_DATA);
    SerialLine.print("Consumed: ");
    SerialLine.println((const char *)data);
  }
  PT_END(&self->pt);
}

AUTOSTART_PROCESSES(&generator_process, &consumer_process);

void setup()
{
  SerialLine.begin(9600);
  process_init();
  autostart_start();
  process_pipe_connect(&generator_process, &consumer_process);
}

void loop()
{
  process_run();
  delay(1);
}