/**
 *   @author http://github.com/jklarenbeek
 *
 *	This demo will not work correctly on SimulIde, since it appears not to support unicode in its serial monitor.
 *	SimulIde also has a problem with flushing/sending its buffer in the same way as a real arduino does.
 */ 

#include <protoduino.h>
#include <lib/ringb8.h>
#include <sys/uart.h>

void delay(unsigned long ms);

RINGB8(echo_rx, SERIAL_RX_BUFFER_SIZE);
RINGB8(echo_tx, SERIAL_TX_BUFFER_SIZE);

static bool echo_rx_buffer = false;

static void on_rx_complete(uint_fast8_t data)
{
  // is there room in the buffer?
  uint_fast8_t available = ringb8_available(&VAR_RINGB8(echo_rx));
  if (available > 0)
  {
    // add the byte to the buffer
    ringb8_put(&VAR_RINGB8(echo_rx), data);
  }

  echo_rx_buffer = true; 
}

static uint32_t errcnt = 0;
static void on_rx_error(uint_fast8_t err)
{
  errcnt++;
  uart0_rx_clear_errors();
}

static int_fast16_t on_tx_complete(void)
{
  // is there anything to transmit?
  uint_fast8_t cnt = ringb8_count(&VAR_RINGB8(echo_tx));
  if (cnt == 0)
    return -1;
    
  // send the next byte from the buffer
  return ringb8_get(&VAR_RINGB8(echo_tx));
}


void setup()
{
  uart0_on_rx_complete(on_rx_complete);
  uart0_on_rx_error(on_rx_error);
  uart0_on_tx_complete(on_tx_complete);
  uart0_open(9600);
}

static void myprint(const char * str)
{
    uint8_t idx = 0;
    while(!(str[idx] == 0 || str[idx] == 13))
    {
      if (ringb8_available(&VAR_RINGB8(echo_tx)) == 0)
        break;
        
      ringb8_put(&VAR_RINGB8(echo_tx), str[idx++]);
    }
}

static void flush(void)
{
    uart0_tx_enable_int();
    while(ringb8_count(&VAR_RINGB8(echo_tx)) > 0)
    {
      delay(1);
    }
}

void loop()
{
  if (uart0_tx_is_available() && echo_rx_buffer == true)
  {
    echo_rx_buffer = false;
    myprint("echo ");
    while(ringb8_count(&VAR_RINGB8(echo_rx)) > 0)
    {
        if (ringb8_available(&VAR_RINGB8(echo_tx)))
        {
          ringb8_put(&VAR_RINGB8(echo_tx), ringb8_get(&VAR_RINGB8(echo_rx)));
        }
    }
    
    //printb(&VAR_RINGB8(echo_rx));
    flush();
  }
  
}
