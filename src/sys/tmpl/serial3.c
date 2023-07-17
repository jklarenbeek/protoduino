#include "../protoduino-config.h"
#include "../serial.h"

#if defined(HAVE_HW_UART3)
#undef CC_TMPL_PREFIX
#undef CC_TMPL2_PREFIX
#define CC_TMPL_PREFIX serial3
#define CC_TMPL2_PREFIX uart3

#include "serial_private_impl.h"

#endif /* HAVE_HW_UART1 */
