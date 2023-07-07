#include "uart.h"

#if defined(HAVE_HW_UART0)
#undef CC_TMPL_PREFIX
#define CC_TMPL_PREFIX uart0

#include <avr/interrupt.h>
#include <util/atomic.h>

#if defined(UBRR0H)

// USART baud rate register high 
#define __UBRRH__ UBRR0H
// USART baud rate register low
#define __UBRRL__ UBRR0L
// UCSRnA – USART Control and Status Register n A
#define __UCSRA__ UCSR0A
// UCSRnB – USART Control and Status Register n B
#define __UCSRB__ UCSR0B
// UCSRnC – USART Control and Status Register n C
#define __UCSRC__ UCSR0C
// USART I/O data register
#define __UDR__ UDR0


//
// Flags for the UCSR0A register
//
// set recieve complete flag (RXC0 = 1)
#define __RXC__ RXC0
// set transmit complete flag (TXC0 = 1)
#define __TXC__ TXC0
// Data Register Empty tells you when the transmit buffer is able to accept another byte (at which time the actual shift register is still shifting out the previous byte.)
#define __UDRE__ UDRE0
// clear Frame Error flag (FE0 = 0)
#define __FE__ FE0
// clear Data overrun flag (DOR0 = 0)
#define __DOR__ DOR0
// clear Parity overrun flag (UPE0 = 0)
#define __UPE__ UPE0
// disable doubling of USART transmission speed (U2X0 = 0)
#define __U2X__ U2X0
#if defined(MPCM0)
// Disable Multi-Processor Communication Mode (MCPM0 = 0)
#define __MPCM__ MPCM0
#endif

//
// Flags for the UCSR0B register
//
// Enable Receive Interrupt (RXCIE0 = 1)
#define __RXCIE__ RXCIE0
// Disable Tranmission Interrupt (TXCIE = 0)
#define __TXCIE__ TXCIE0
// Disable Data Register Empty interrupt (UDRIE0 = 0)
#define __UDRIE__ UDRIE0
// Enable reception (RXEN0 = 1)
#define __RXEN__ RXEN0
// Enable transmission (TXEN0 = 1)
#define __TXEN__ TXEN0
// Set 8-bit character mode (UCSZ00, UCSZ01, and UCSZ02 together control this, But UCSZ00, UCSZ01 are in Register UCSR0C)
#define __UCSZ2__ UCSZ02
// Bit 1 – RXB8n: Receive Data Bit 8 n
#define __RXB8__ RXB80
// Bit 0 – TXB8n: Transmit Data Bit 8 n
#define __TXB8__ TXB80

//
// Flags for the UCSR0C register
//
// USART Mode select -- UMSEL00 = 0 and UMSEL01 = 0 for asynchronous mode
#define __UMSEL0__ UMSEL00
#define __UMSEL1__ UMSEL01
// disable parity mode -- UPM00 = 0 and UPM01 = 0
#define __UPM0__ UPM00
#define __UPM1__ UPM01
// Set USBS = 1 to configure to 2 stop bits per DMX standard.  The USART receiver ignores this 
// setting anyway, and will only set a frame error flag if the first stop bit is 0.  
#define __USBS__ USBS0
// Finish configuring for 8 data bits by setting UCSZ00 and UCSZ01 to 1
#define __UCSZ0__ UCSZ00
#define __UCSZ1__ UCSZ01
// Set clock parity to 0 for asynchronous mode (UCPOL0 = 0). */
#define __UCPOL__ UCPOL0

#else

#define __UBRRH__ UBRRH
#define __UBRRL__ UBRRL

#define __UCSRA__ UCSRA
#define __UCSRB__ UCSRB
#define __UCSRC__ UCSRC
#define __UDR__ UDR

#define __RXC__ RXC
#define __TXC__ TXC
#define __UDRE__ UDRE
#define __FE__ FE
#define __DOR__ DOR
#define __UPE__ UPE
#define __U2X__ U2X
#if defined(MPCM)
#define __MPCM__ MPCM
#endif

#define __RXCIE__ RXCIE
#define __TXCIE__ TXCIE
#define __UDRIE__ UDRIE
#define __RXEN__ RXEN
#define __TXEN__ TXEN
#define __UCSZ2__ UCSZ2
#define __RXB8__ RXB8
#define __TXB8__ TXB8

#define __UMSEL0__ UMSEL0
#define __UMSEL1__ UMSEL1
#define __UPM0__ UPM0
#define __UPM1__ UPM1
#define __USBS__ USBS
#define __UCSZ0__ UCSZ0
#define __UCSZ1__ UCSZ1
#define __UCPOL__ UCPOL

#endif

#if defined(USART_RX_vect)
#define __ISR_RX_VECT__ USART_RX_vect
#elif defined(USART0_RX_vect)
#define __ISR_RX_VECT__ USART0_RX_vect
#elif defined(USART_RXC_vect)
#define __ISR_RX_VECT__ USART_RXC_vect
#else
  #error "Don't know what Data Received vector is called for uart0"
#endif

#if defined(UART0_UDRE_vect)
#define __ISR_UDRE_VECT__ UART0_UDRE_vect
#elif defined(UART_UDRE_vect)
#define __ISR_UDRE_VECT__ UART_UDRE_vect
#elif defined(USART0_UDRE_vect)
#define __ISR_UDRE_VECT__ USART0_UDRE_vect
#elif defined(USART_UDRE_vect)
#define __ISR_UDRE_VECT__ USART_UDRE_vect
#else
  #error "Don't know what Data Register Empty vector is called for uart0"
#endif

#include "uart_private_impl.h"

#endif /* HAVE_HW_UART0 */
