#include <protoduino.h>
#include <sys/process.h>
#include <sys/autostart.h>
#include <lib/ringb8.h>
#include <sys/uart.h>

// Ring buffers for UART
RINGB8(rx_buf, 32);
RINGB8(tx_buf, 32);

// UART callbacks
static void on_rx(uint8_t data)
{
  ringb8_put(&VAR_RINGB8(rx_buf), data);
  process_poll(&uart_process);
}

static int16_t on_tx()
{
  if (ringb8_count(&VAR_RINGB8(tx_buf)) == 0)
    return -1;
  return ringb8_get(&VAR_RINGB8(tx_buf));
}

// UART Process
PROCESS(uart_process, "UART Echo", 0);
PROCESS_THREAD(uart_process, ev, data)
{
  PT_BEGIN(&self->pt);
  while (1)
  {
    PT_WAIT_UNTIL(&self->pt, ev == PROCESS_EVENT_POLL && ringb8_count(&VAR_RINGB8(rx_buf)) > 0);
    while (ringb8_count(&VAR_RINGB8(rx_buf)) > 0)
    {
      uint8_t ch = ringb8_get(&VAR_RINGB8(rx_buf));
      ringb8_put(&VAR_RINGB8(tx_buf), ch); // Echo
    }
    uart0_tx_enable_int(); // Trigger TX if needed
  }
  PT_END(&self->pt);
}

AUTOSTART_PROCESSES(&uart_process);

void setup()
{
  SerialLine.begin(9600); // But use uart0_ for callbacks
  uart0_on_rx_complete(on_rx);
  uart0_on_tx_complete(on_tx);
  process_init();
  autostart_start();
}
void loop()
{
  process_run();
  delay(1);
}