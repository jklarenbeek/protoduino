// file: ./src/sys/tmpl/serial1.c

#include "../serial.h"

#if defined(HAVE_HW_UART1)
#undef CC_TMPL_PREFIX
#undef CC_TMPL2_PREFIX
#define CC_TMPL_PREFIX serial1
#define CC_TMPL2_PREFIX uart1

#include "serial_private_impl.h"

#endif /* HAVE_HW_UART1 */
