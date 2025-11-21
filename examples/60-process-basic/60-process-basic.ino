#include <protoduino.h>
#include <sys/process.h>
#include <sys/autostart.h>

// Define events
process_event_t custom_event;

// Process A: Sender
PROCESS(sender_process, "Sender", 1);
PROCESS_THREAD(sender_process, ev, data)
{
  static struct etimer timer;
  PT_BEGIN(&self->pt);
  etimer_set(&timer, CLOCK_SECOND * 2);
  while (1)
  {
    PT_WAIT_UNTIL(&self->pt, etimer_expired(&timer));
    process_post(&receiver_process, custom_event, (process_data_t) "Hello from sender!");
    etimer_reset(&timer);
  }
  PT_END(&self->pt);
}

// Process B: Receiver
PROCESS(receiver_process, "Receiver", 2);
PROCESS_THREAD(receiver_process, ev, data)
{
  PT_BEGIN(&self->pt);
  while (1)
  {
    PT_WAIT_UNTIL(&self->pt, ev == custom_event);
    SerialLine.print("Received: ");
    SerialLine.println((const char *)data);
  }
  PT_END(&self->pt);
}

AUTOSTART_PROCESSES(&sender_process, &receiver_process);

void setup()
{
  SerialLine.begin(9600);
  process_init();
  custom_event = process_alloc_event();
  autostart_start();
}

void loop()
{
  process_run();
  delay(1); // Small delay to avoid busy loop
}