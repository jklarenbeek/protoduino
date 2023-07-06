#include "uart.h"

#if defined(HAVE_HW_UART2)
#undef CC_NAME_PREFIX
#define CC_NAME_PREFIX uart2

#include <avr/interrupt.h>
#include <util/atomic.h>

// USART baud rate register high 
#define __UBRRH__ UBRR2H
// USART baud rate register low
#define __UBRRL__ UBRR2L
// UCSRnA – USART Control and Status Register n A
#define __UCSRA__ UCSR2A
// UCSRnB – USART Control and Status Register n B
#define __UCSRB__ UCSR2B
// UCSRnC – USART Control and Status Register n C
#define __UCSRC__ UCSR2C
// USART I/O data register
#define __UDR__ UDR2


//
// Flags for the UCSR0A register
//
// set recieve complete flag (RXC = 1)
#define __RXC__ RXC2
// set transmit complete flag (TXC = 1)
#define __TXC__ TXC2
// Data Register Empty tells you when the transmit buffer is able to accept another byte (at which time the actual shift register is still shifting out the previous byte.)
#define __UDRE__ UDRE2
// clear Frame Error flag (FE = 0)
#define __FE__ FE2
// clear Data overrun flag (DOR = 0)
#define __DOR__ DOR2
// clear Parity overrun flag (UPE = 0)
#define __UPE__ UPE2
// disable doubling of USART transmission speed (U2X = 0)
#define __U2X__ U2X2
#if defined(MPCM2)
// Disable Multi-Processor Communication Mode (MCPM = 0)
#define __MPCM__ MPCM2
#endif

//
// Flags for the UCSRB register
//
// Enable Receive Interrupt (RXCIE = 1)
#define __RXCIE__ RXCIE2
// Disable Tranmission Interrupt (TXCIE = 0)
#define __TXCIE__ TXCIE2
// Disable Data Register Empty interrupt (UDRIE = 0)
#define __UDRIE__ UDRIE2
// Enable reception (RXEN = 1)
#define __RXEN__ RXEN2
// Enable transmission (TXEN = 1)
#define __TXEN__ TXEN2
// Set 8-bit character mode (UCSZ0, UCSZ1, and UCSZ2 together control this, But UCSZ0, UCSZ1 are in Register UCSRC)
#define __UCSZ2__ UCSZ22
// Bit 1 – RXB8n: Receive Data Bit 8 n
#define __RXB8__ RXB82
// Bit 0 – TXB8n: Transmit Data Bit 8 n
#define __TXB8__ TXB82

//
// Flags for the UCSR0C register
//
// USART Mode select -- UMSEL20 = 0 and UMSEL21 = 0 for asynchronous mode
#define __UMSEL0__ UMSEL20
#define __UMSEL1__ UMSEL21
// disable parity mode -- UPM20 = 0 and UPM21 = 0
#define __UPM0__ UPM20
#define __UPM1__ UPM21
// Set USBS = 1 to configure to 2 stop bits per DMX standard.  The USART receiver ignores this 
// setting anyway, and will only set a frame error flag if the first stop bit is 0.  
#define __USBS__ USBS2
// Finish configuring for 8 data bits by setting UCSZ0 and UCSZ1 to 1
#define __UCSZ0__ UCSZ20
#define __UCSZ1__ UCSZ21
// Set clock parity to 0 for asynchronous mode (UCPOL = 0). */
#define __UCPOL__ UCPOL2


#if defined(UART2_RX_vect)
#define __ISR_RX_VECT__ UART2_RX_vect
#elif defined(USART2_RX_vect)
#define __ISR_RX_VECT__ USART2_RX_vect
#else
  #error "Don't know what Data Received vector is called for uart2"
#endif

#if defined(UART2_UDRE_vect)
#define __ISR_UDRE_VECT__ UART2_UDRE_vect
#elif defined(USART2_UDRE_vect)
#define __ISR_UDRE_VECT__ USART2_UDRE_vect
#else
  #error "Don't know what Data Register Empty vector is called for uart2"
#endif

#include "uart_private_impl.h"

#endif /* HAVE_HW_UART2 */
