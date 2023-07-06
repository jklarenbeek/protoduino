#include "uart.h"

#if defined(HAVE_HW_UART1)
#undef CC_NAME_PREFIX
#define CC_NAME_PREFIX uart1

#include <avr/interrupt.h>
#include <util/atomic.h>

// USART baud rate register high 
#define __UBRRH__ UBRR1H
// USART baud rate register low
#define __UBRRL__ UBRR1L
// UCSRnA – USART Control and Status Register n A
#define __UCSRA__ UCSR1A
// UCSRnB – USART Control and Status Register n B
#define __UCSRB__ UCSR1B
// UCSRnC – USART Control and Status Register n C
#define __UCSRC__ UCSR1C
// USART I/O data register
#define __UDR__ UDR1


//
// Flags for the UCSR0A register
//
// set recieve complete flag (RXC1 = 1)
#define __RXC__ RXC1
// set transmit complete flag (TXC1 = 1)
#define __TXC__ TXC1
// Data Register Empty tells you when the transmit buffer is able to accept another byte (at which time the actual shift register is still shifting out the previous byte.)
#define __UDRE__ UDRE1
// clear Frame Error flag (FE1 = 0)
#define __FE__ FE1
// clear Data overrun flag (DOR1 = 0)
#define __DOR__ DOR1
// clear Parity overrun flag (UPE1 = 0)
#define __UPE__ UPE1
// disable doubling of USART transmission speed (U2X1 = 0)
#define __U2X__ U2X1
#if defined(MPCM1)
// Disable Multi-Processor Communication Mode (MCPM0 = 0)
#define __MPCM__ MPCM1
#endif

//
// Flags for the UCSR0B register
//
// Enable Receive Interrupt (RXCIE1 = 1)
#define __RXCIE__ RXCIE1
// Disable Tranmission Interrupt (TXCIE1 = 0)
#define __TXCIE__ TXCIE1
// Disable Data Register Empty interrupt (UDRIE1 = 0)
#define __UDRIE__ UDRIE1
// Enable reception (RXEN1 = 1)
#define __RXEN__ RXEN1
// Enable transmission (TXEN1 = 1)
#define __TXEN__ TXEN1
// Set 8-bit character mode (UCSZ10, UCSZ11, and UCSZ12 together control this, But UCSZ10, UCSZ11 are in Register UCSR1C)
#define __UCSZ2__ UCSZ12
// Bit 1 – RXB8n: Receive Data Bit 8 n
#define __RXB8__ RXB81
// Bit 0 – TXB8n: Transmit Data Bit 8 n
#define __TXB8__ TXB81

//
// Flags for the UCSR0C register
//
// USART Mode select -- UMSEL10 = 0 and UMSEL11 = 0 for asynchronous mode
#define __UMSEL0__ UMSEL10
#define __UMSEL1__ UMSEL11
// disable parity mode -- UPM10 = 0 and UPM11 = 0
#define __UPM0__ UPM10
#define __UPM1__ UPM11
// Set USBS = 1 to configure to 2 stop bits per DMX standard.  The USART receiver ignores this 
// setting anyway, and will only set a frame error flag if the first stop bit is 0.  
#define __USBS__ USBS1
// Finish configuring for 8 data bits by setting UCSZ00 and UCSZ01 to 1
#define __UCSZ0__ UCSZ10
#define __UCSZ1__ UCSZ11
// Set clock parity to 0 for asynchronous mode (UCPOL0 = 0). */
#define __UCPOL__ UCPOL1


#if defined(UART1_RX_vect)
#define __ISR_RX_VECT__ UART1_RX_vect
#elif defined(USART1_RX_vect)
#define __ISR_RX_VECT__ USART1_RX_vect
#else
  #error "Don't know what Data Received vector is called for uart1"
#endif

#if defined(UART1_UDRE_vect)
#define __ISR_UDRE_VECT__ UART1_UDRE_vect
#elif defined(USART1_UDRE_vect)
#define __ISR_UDRE_VECT__ USART1_UDRE_vect
#else
  #error "Don't know what Data Register Empty vector is called for uart1"
#endif

#include "uart_private_impl.h"

#endif /* HAVE_HW_UART1 */
