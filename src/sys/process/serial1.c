// file: ./src/sys/process/serial1.c
// Serial device process implementation for UART1.

#include "serial.h"

#if defined(HAVE_HW_UART1)

#undef  CC_TMPL_PREFIX
#undef  CC_TMPL2_PREFIX
#define CC_TMPL_PREFIX          serial_process1
#define CC_TMPL2_PREFIX         serial1
#define SERIAL_PROCESS_NAME_STR "serial:1"

#include "serial_private.h"

#undef SERIAL_PROCESS_NAME_STR

#endif /* HAVE_HW_UART1 */
