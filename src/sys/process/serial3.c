// file: ./src/sys/process/serial3.c
// Serial device process implementation for UART3.

#include "serial.h"

#if defined(HAVE_HW_UART3)

#undef  CC_TMPL_PREFIX
#undef  CC_TMPL2_PREFIX
#define CC_TMPL_PREFIX          serial_process3
#define CC_TMPL2_PREFIX         serial3
#define SERIAL_PROCESS_NAME_STR "serial:3"

#include "serial_private.h"

#undef SERIAL_PROCESS_NAME_STR

#endif /* HAVE_HW_UART3 */
