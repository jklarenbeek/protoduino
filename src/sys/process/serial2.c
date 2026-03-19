// file: ./src/sys/process/serial2.c
// Serial device process implementation for UART2.

#include "serial.h"

#if defined(HAVE_HW_UART2)

#undef  CC_TMPL_PREFIX
#undef  CC_TMPL2_PREFIX
#define CC_TMPL_PREFIX          serial_process2
#define CC_TMPL2_PREFIX         serial2
#define SERIAL_PROCESS_NAME_STR "serial:2"

#include "serial_private.h"

#undef SERIAL_PROCESS_NAME_STR

#endif /* HAVE_HW_UART2 */
