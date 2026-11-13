// file: ./src/sys/settings.h

#ifndef __SETTINGS_H__
#define __SETTINGS_H__

/* Settings: the numbers a sketch runs on.
 *
 * A setting is one byte with a name. Its value is built in, and it can be
 * changed on the finished device without building anything: the changed
 * values live in EEPROM, and a program on the PC writes them there through
 * the port (src/mod/port.h, "set" in tools/port.mjs).
 *
 * The sketch lists its settings once, before it includes this header:
 *
 *   #define SETTINGS(X) \
 *     X(period, 30, "ticks between two samples") \
 *     X(low,    27, "tenths of a volt below which the supply is low")
 *   #include <sys/settings.h>
 *
 *   setting(SETTING_period)        // the value in force
 *
 * - The values are as built until somebody changes one. From then on
 *   EEPROM holds all of them, with a mark behind them that says so; a part
 *   that was never set, or whose list of settings has changed in length,
 *   runs on the values as built.
 * - They start at EEPROM address SETTINGS_CONF_AT, 0 by default, and take
 *   one byte each and one for the mark.
 * - The names and the help texts are for the PC. They are kept in the .elf
 *   file, in a section that is not loaded, and cost the part nothing.
 * - A sketch that copies settings into its processes does so in a function
 *   called settings_apply(); the port calls it after the PC has changed a
 *   setting. Writing the settings is the one change a port allows at its
 *   level 2 (src/mod/port.h).
 */

#include <protoduino.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>

#ifndef SETTINGS
#error "settings: list them with #define SETTINGS(X) before this header"
#endif

#ifndef SETTINGS_CONF_AT
#define SETTINGS_CONF_AT 0
#endif

#define SETTINGS_MARK 0x5E

#define SETTINGS_STR_(x) #x
#define SETTINGS_STR(x)  SETTINGS_STR_(x)

// a text for the PC: kept in the .elf file, not in the part
#define SETTINGS_NOTE(text) \
  __asm__(".pushsection .protoduino.notes,\"\",@progbits\n.asciz \"" text "\"\n.popsection\n")

#define SETTINGS_INDEX(name, value, help)   SETTING_##name,
#define SETTINGS_DEFAULT(name, value, help) (value),
#define SETTINGS_TEXT(name, value, help)    SETTINGS_NOTE("setting\\t" #name "\\t" help);

enum { SETTINGS(SETTINGS_INDEX) SETTINGS_COUNT };

#ifdef __cplusplus
extern "C" {
#endif
extern const uint8_t settings_defaults[];
#ifdef __cplusplus
}
#endif
const uint8_t settings_defaults[] CC_PROGMEM = { SETTINGS(SETTINGS_DEFAULT) };

/* The sketch may define this, to copy settings to where they are used. It
 * is declared here so that it keeps its plain name and stays a function of
 * its own, which the PC can call. */
#ifdef __cplusplus
extern "C" {
#endif
void settings_apply(void) __attribute__((weak, used, noinline));
#ifdef __cplusplus
}
#endif

SETTINGS_NOTE("settings\\t" SETTINGS_STR(SETTINGS_CONF_AT));
SETTINGS(SETTINGS_TEXT)

/**
 * The value of a setting: SETTING_<name>.
 */
static uint8_t setting(uint8_t index)
{
  const uint8_t *at = (const uint8_t *)(SETTINGS_CONF_AT);

  if (eeprom_read_byte(at + SETTINGS_COUNT) == (uint8_t)(SETTINGS_MARK ^ SETTINGS_COUNT))
    return eeprom_read_byte(at + index);
  return pgm_read_byte(&settings_defaults[index]);
}

#endif /* __SETTINGS_H__ */
