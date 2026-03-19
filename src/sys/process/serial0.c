// file: ./src/sys/process/serial0.c
// Serial device process implementation for UART0.

#include "serial.h"

#if defined(HAVE_HW_UART0)

#undef  CC_TMPL_PREFIX
#undef  CC_TMPL2_PREFIX
#define CC_TMPL_PREFIX          serial_process0
#define CC_TMPL2_PREFIX         serial0
#define SERIAL_PROCESS_NAME_STR "serial:0"

#include "serial_private.h"

#undef SERIAL_PROCESS_NAME_STR

#endif /* HAVE_HW_UART0 */
