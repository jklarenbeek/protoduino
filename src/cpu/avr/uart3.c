// file: ./src/cpu/avr/uart3.c

#include "uart.h"

#if defined(HAVE_HW_UART3)
#undef CC_TMPL_PREFIX
#define CC_TMPL_PREFIX uart3

#include <avr/interrupt.h>
#include <util/atomic.h>

// USART baud rate register high
#define __UBRRH__ UBRR3H
// USART baud rate register low
#define __UBRRL__ UBRR3L
// UCSRnA – USART Control and Status Register n A
#define __UCSRA__ UCSR3A
// UCSRnB – USART Control and Status Register n B
#define __UCSRB__ UCSR3B
// UCSRnC – USART Control and Status Register n C
#define __UCSRC__ UCSR3C
// USART I/O data register
#define __UDR__ UDR3


//
// Flags for the UCSR0A register
//
// set recieve complete flag (RXC = 1)
#define __RXC__ RXC3
// set transmit complete flag (TXC = 1)
#define __TXC__ TXC3
// Data Register Empty tells you when the transmit buffer is able to accept another byte (at which time the actual shift register is still shifting out the previous byte.)
#define __UDRE__ UDRE3
// clear Frame Error flag (FE = 0)
#define __FE__ FE3
// clear Data overrun flag (DOR = 0)
#define __DOR__ DOR3
// clear Parity overrun flag (UPE = 0)
#define __UPE__ UPE3
// disable doubling of USART transmission speed (U2X = 0)
#define __U2X__ U2X3
#if defined(MPCM3)
// Disable Multi-Processor Communication Mode (MCPM = 0)
#define __MPCM__ MPCM3
#endif

//
// Flags for the UCSRB register
//
// Enable Receive Interrupt (RXCIE = 1)
#define __RXCIE__ RXCIE3
// Disable Tranmission Interrupt (TXCIE = 0)
#define __TXCIE__ TXCIE3
// Disable Data Register Empty interrupt (UDRIE = 0)
#define __UDRIE__ UDRIE3
// Enable reception (RXEN = 1)
#define __RXEN__ RXEN3
// Enable transmission (TXEN = 1)
#define __TXEN__ TXEN3
// Set 8-bit character mode (UCSZ0, UCSZ1, and UCSZ2 together control this, But UCSZ0, UCSZ1 are in Register UCSRC)
#define __UCSZ2__ UCSZ32
// Bit 1 – RXB8n: Receive Data Bit 8 n
#define __RXB8__ RXB83
// Bit 0 – TXB8n: Transmit Data Bit 8 n
#define __TXB8__ TXB83

//
// Flags for the UCSRC register
//
// USART Mode select -- UMSEL30 = 0 and UMSEL31 = 0 for asynchronous mode
#define __UMSEL0__ UMSEL30
#define __UMSEL1__ UMSEL31
// disable parity mode -- UPM30 = 0 and UPM31 = 0
#define __UPM0__ UPM30
#define __UPM1__ UPM31
// Set USBS = 1 to configure to 2 stop bits per DMX standard.  The USART receiver ignores this
// setting anyway, and will only set a frame error flag if the first stop bit is 0.
#define __USBS__ USBS3
// Finish configuring for 8 data bits by setting UCSZ0 and UCSZ1 to 1
#define __UCSZ0__ UCSZ30
#define __UCSZ1__ UCSZ31
// Set clock parity to 0 for asynchronous mode (UCPOL = 0). */
#define __UCPOL__ UCPOL3


#if defined(UART3_RX_vect)
#define __ISR_RX_VECT__ UART3_RX_vect
#elif defined(USART3_RX_vect)
#define __ISR_RX_VECT__ USART3_RX_vect
#else
  #error "Don't know what Data Received vector is called for uart3"
#endif

#if defined(UART3_UDRE_vect)
#define __ISR_UDRE_VECT__ UART3_UDRE_vect
#elif defined(USART3_UDRE_vect)
#define __ISR_UDRE_VECT__ USART3_UDRE_vect
#else
  #error "Don't know what Data Register Empty vector is called for uart3"
#endif

#include "uart_private_impl.h"

#endif /* HAVE_HW_UART3 */
