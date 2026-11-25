// file: ./src/cpu/rp2/uart1.c

#include <cc.h>

#if defined(CC_ARCH_RP2)
#include "hardware/uart.h"
#include "uart.h"

/* the SDK's instance, taken before its macro uart1 is put away: the
   names made by CC_TMPL_FN need the bare word */
static uart_inst_t *const rp2_uart1 = uart1;
#undef uart0
#undef uart1

#undef CC_TMPL_PREFIX
#define CC_TMPL_PREFIX uart1
#define __UART__ rp2_uart1
#define __UART_IRQ__ UART1_IRQ
#define __UART_TX__ UART_CONF_UART1_TX
#define __UART_RX__ UART_CONF_UART1_RX

#include "uart_private_impl.h"

#endif /* CC_ARCH_RP2 */
