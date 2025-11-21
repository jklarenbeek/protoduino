// file: ./src/sys/tmpl/serial2.c

#include "../serial.h"

#if defined(HAVE_HW_UART2)
#undef CC_TMPL_PREFIX
#undef CC_TMPL2_PREFIX
#define CC_TMPL_PREFIX serial2
#define CC_TMPL2_PREFIX uart2

#include "serial_private_impl.h"

#endif /* HAVE_HW_UART2 */
