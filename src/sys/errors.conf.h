// file: ./src/sys/errors.conf.h

#ifndef __ERRORS_CONF_H__
#define __ERRORS_CONF_H__

// --------------------------------------------------------------------------
// Configuration: Select message set (override in protoduino-config.h)
// --------------------------------------------------------------------------

// 0: Short strings for constrained memory (ATTiny, Arduino Uno).
// 1: Verbose strings for systems with more memory (e.g., 32KB+ flash).
#ifndef ERRORS_CONF_VERBOSE_MESSAGES
#define ERRORS_CONF_VERBOSE_MESSAGES 0
#endif

#endif