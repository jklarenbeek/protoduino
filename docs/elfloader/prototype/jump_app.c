/* jump_app.c - an application that asks to be replaced: it jumps to the bootloader.
 * It does so in the state a running system is in: interrupts on, timer 0 overflowing
 * into a handler, the UART at another baud rate with its receive interrupt enabled.
 * Build with -DBOOT_START=<byte address of the bootloader>. */
#include <avr/io.h>
#include <avr/interrupt.h>

static volatile uint8_t ticks;
ISR(TIMER0_OVF_vect) { ticks++; }
ISR(USART_RX_vect) { (void)UDR0; }

static void put(char c) { while (!(UCSR0A & _BV(UDRE0))) { } UDR0 = c; }

int main(void)
{
  UCSR0A = _BV(U2X0); UBRR0H = 0; UBRR0L = 16; UCSR0B = _BV(TXEN0);
  put('['); put('J'); put(']');
  while (!(UCSR0A & _BV(TXC0))) { }

  /* a system at work */
  UCSR0A = 0; UBRR0H = 0x0D; UBRR0L = 0x04;                 /* 300 baud */
  UCSR0B = _BV(RXEN0) | _BV(TXEN0) | _BV(RXCIE0);
  TCCR0B = _BV(CS00); TIMSK0 = _BV(TOIE0);
  sei();
  while (ticks < 20) { }

  ((void (*)(void))(BOOT_START / 2))();                     /* no reset: MCUSR stays as the bootloader left it */
  for (;;) { }
}
