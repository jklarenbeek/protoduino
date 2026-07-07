// file: examples/26-rune-conformance/26-rune-conformance.ino

/*
 * 26-rune-conformance.ino  –  Unicode layer conformance test (on target)
 * ======================================================================
 *
 * Exercises the rune16 / rune32 / utf8 stack ON THE AVR ITSELF, where the
 * PROGMEM table access actually matters (host builds read the tables via
 * plain dereference and cannot catch pgm_read_* bugs).  Prints PASS/FAIL
 * per check and a final TOTALS line, then the <DONE> sentinel.
 *
 * Covers the historical failure modes of this layer:
 *   - PROGMEM tables read without pgm_read_word()  (garbage on AVR only)
 *   - signed rune16_t breaking binary search and encode for U+8000+
 *     (Hangul, CJK-compat, fullwidth forms)
 *   - swapped case-conversion wrappers (utf8_toupper lowercased, …)
 *   - overlong UTF-8 encodings accepted by utf8_valid()
 *
 * Run head-less under protosim:
 *
 *   arduino-cli compile --fqbn arduino:avr:uno --library . \
 *       --build-path build/26 examples/26-rune-conformance
 *   protosim build/26/26-rune-conformance.ino.elf -m atmega328p -f 16000000 \
 *       --uart0-out out.txt --exit-on-uart "<DONE>" --max-steps 80000000
 *
 * Expected: every line PASS, "TOTALS pass=NN fail=0".
 */

#include <protoduino.h>
#include <sys/uart.h>

#include <lib/text/utf8.h>      /* brings rune16.h + rune32.h */
#include <lib/text/utf8_iter.h>

#include <string.h>

/* ---- polled blocking UART0 output (platform uart API) ---- */
static void up(uint8_t b) { while (!uart0_tx_is_available()) { } uart0_tx_write8(b); }

static void us_P(const char *s_P)
{
    char c;
    while ((c = (char)pgm_read_byte(s_P++)) != 0)
        up((uint8_t)c);
}

static void udec(uint16_t v)
{
    char buf[6]; uint8_t i = sizeof(buf); buf[--i] = '\0';
    if (!v) { up('0'); return; }
    while (v && i) { buf[--i] = (char)('0' + v % 10); v /= 10; }
    while (buf[i]) up((uint8_t)buf[i++]);
}

static uint16_t pass_n = 0, fail_n = 0;

static void check_P(bool ok, const char *label_P)
{
    us_P(ok ? PSTR("PASS ") : PSTR("FAIL "));
    us_P(label_P);
    up('\n');
    if (ok) ++pass_n; else ++fail_n;
}

#define CHK(cond, label)  check_P((cond), PSTR(label))

/* Encode + decode one rune16 and require an exact round trip. */
static bool rt16(rune16_t r, uint8_t want_len)
{
    char     b[4];
    rune16_t d;
    uint8_t  n = utf8_fromrune16(b, r);
    if (n != want_len) return false;
    return utf8_torune16(&d, b) == n && d == r;
}

void setup(void)
{
    uart0_open(UART_BAUD_9600);

    us_P(PSTR("== rune/utf8 conformance ==\n"));

    /* ---- rune16 case conversion (tables in PROGMEM) ---- */
    CHK(rune16_tolower(0x0041u) == 0x0061u, "tolower A->a");
    CHK(rune16_toupper(0x0061u) == 0x0041u, "toupper a->A");
    CHK(rune16_tolower(0x00C9u) == 0x00E9u, "tolower E-acute");
    CHK(rune16_toupper(0x00E9u) == 0x00C9u, "toupper e-acute");
    CHK(rune16_tolower(0x03A9u) == 0x03C9u, "tolower Omega");
    CHK(rune16_toupper(0x044Fu) == 0x042Fu, "toupper ya (cyrillic)");
    CHK(rune16_tolower(0x0130u) == 0x0069u, "tolower I-dot (singlet)");
    CHK(rune16_toupper(0x00FFu) == 0x0178u, "toupper y-diaeresis (singlet)");
    CHK(rune16_totitle(0x01C6u) == 0x01C5u, "totitle dz-caron");
    CHK(rune16_tolower(0x0058u) == 0x0078u, "tolower X->x");
    CHK(rune16_tolower(0x0061u) == 0x0061u, "tolower a unchanged");
    CHK(rune16_toupper(0x0041u) == 0x0041u, "toupper A unchanged");

    /* High-BMP case pairs: break when rune16_t is signed */
    CHK(rune16_tolower(0xFF21u) == 0xFF41u, "tolower fullwidth A");
    CHK(rune16_toupper(0xFF41u) == 0xFF21u, "toupper fullwidth a");
    CHK(rune16_tolower(0x2160u) == 0x2170u, "tolower roman numeral I");

    /* ---- rune16 classification ---- */
    CHK(rune16_isalpha(0x0041u),  "isalpha A");
    CHK(rune16_isalpha(0x007Au),  "isalpha z");
    CHK(rune16_isalpha(0x00E9u),  "isalpha e-acute");
    CHK(rune16_isalpha(0x4E00u),  "isalpha CJK 4E00");
    CHK(rune16_isalpha(0xAC00u),  "isalpha hangul AC00");
    CHK(rune16_isalpha(0xFF41u),  "isalpha fullwidth a");
    CHK(!rune16_isalpha(0x0030u), "isalpha 0 false");
    CHK(!rune16_isalpha(0x3000u), "isalpha ideo-space false");

    CHK(rune16_isspace(0x0020u),  "isspace SP");
    CHK(rune16_isspace(0x0009u),  "isspace TAB");
    CHK(rune16_isspace(0x00A0u),  "isspace NBSP");
    CHK(rune16_isspace(0x3000u),  "isspace ideographic");
    CHK(rune16_isspace(0xFEFFu),  "isspace BOM/ZWNBSP");
    CHK(!rune16_isspace(0x0078u), "isspace x false");

    CHK(rune16_isupper(0x0041u),  "isupper A");
    CHK(rune16_isupper(0xFF21u),  "isupper fullwidth A");
    CHK(!rune16_isupper(0x0061u), "isupper a false");
    CHK(rune16_islower(0x0061u),  "islower a");
    CHK(rune16_islower(0xFF41u),  "islower fullwidth a");
    CHK(!rune16_islower(0x0041u), "islower A false");

    /* ---- utf8 codec round trips (encode breaks when rune16_t signed) ---- */
    CHK(rt16(0x0024u, 1), "roundtrip $ 1B");
    CHK(rt16(0x00A2u, 2), "roundtrip cent 2B");
    CHK(rt16(0x20ACu, 3), "roundtrip euro 3B");
    CHK(rt16(0x4E00u, 3), "roundtrip CJK 3B");
    CHK(rt16(0xFF41u, 3), "roundtrip fullwidth 3B");
    {
        char sb[4];
        CHK(utf8_fromrune16(sb, 0xD800u) == 0u, "encode surrogate rejected");
    }

    /* ---- utf8 case conversion (was swapped: toupper lowercased) ---- */
    {
        char out[16];
        CHK(utf8_toupper(out, sizeof(out), "h\xC3\xA9llo")
            && strcmp(out, "H\xC3\x89LLO") == 0, "utf8_toupper hello");
        CHK(utf8_tolower(out, sizeof(out), "H\xC3\x89LLO")
            && strcmp(out, "h\xC3\xA9llo") == 0, "utf8_tolower HELLO");
    }

    /* ---- utf8_valid: overlong + surrogate + emoji ---- */
    CHK(utf8_valid("abc"),                  "valid ascii");
    CHK(utf8_valid("h\xC3\xA9llo"),         "valid latin1");
    CHK(utf8_valid("\xF0\x9F\x98\x80"),     "valid emoji");
    CHK(!utf8_valid("\xC0\x80"),            "overlong 2B rejected");
    CHK(!utf8_valid("\xE0\x80\x80"),        "overlong 3B rejected");
    CHK(!utf8_valid("\xF0\x80\x80\x80"),    "overlong 4B rejected");
    CHK(!utf8_valid("\xED\xA0\x80"),        "surrogate rejected");
    CHK(!utf8_valid("\xFF"),                "invalid lead rejected");

    /* ---- rune32 wrappers (were swapped) + emoji classification ---- */
    CHK(rune32_toupper((rune32_t)'a') == (rune32_t)'A', "rune32_toupper a->A");
    CHK(rune32_tolower((rune32_t)'A') == (rune32_t)'a', "rune32_tolower A->a");
    CHK(rune32_toupper(0xFF41u) == 0xFF21u,  "rune32_toupper fullwidth");
    CHK(rune32_isemoji(0x1F600u),            "isemoji grinning face");
    CHK(rune32_isemoji(0x2B50u),             "isemoji star (BMP)");
    CHK(!rune32_isemoji((rune32_t)'A'),      "isemoji A false");
    CHK(rune32_isalpha(0x4E00u),             "rune32_isalpha CJK");
    CHK(rune32_toupper(0x1F600u) == 0x1F600u, "rune32 SMP no case");

    /* ---- 32-bit string ops ---- */
    CHK(utf8_strlen32("a\xF0\x9F\x98\x80" "b") == 3, "strlen32 a emoji b");
    CHK(utf8_strlen("h\xC3\xA9llo") == 5,            "strlen hello");

    us_P(PSTR("TOTALS pass="));
    udec(pass_n);
    us_P(PSTR(" fail="));
    udec(fail_n);
    up('\n');
    us_P(PSTR("<DONE>\n"));
}

void loop(void)
{
    /* everything happened in setup() */
}
