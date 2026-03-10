#include <sys/errors.h>
#include "vterm.h"
#include "utf8.h"

// VT400 INPUT CODES
const char VT_KEY_NM_UP[]        CC_PROGMEM = "[A";
const char VT_KEY_NM_DOWN[]      CC_PROGMEM = "[B";
const char VT_KEY_NM_RIGHT[]     CC_PROGMEM = "[C";
const char VT_KEY_NM_LEFT[]      CC_PROGMEM = "[D";
const char VT_KEY_NM_HOME[]      CC_PROGMEM = "[1~";
const char VT_KEY_NM_IC[]        CC_PROGMEM = "[2~";
const char VT_KEY_NM_DC[]        CC_PROGMEM = "[3~";
const char VT_KEY_NM_END[]       CC_PROGMEM = "[4~";
const char VT_KEY_NM_PPAGE[]     CC_PROGMEM = "[5~";
const char VT_KEY_NM_NPAGE[]     CC_PROGMEM = "[6~";
const char VT_KEY_NM_CENTER[]    CC_PROGMEM = "[G";
const char VT_KEY_NM_BTAB[]      CC_PROGMEM = "[Z";
const char VT_KEY_NM_HOME_H[]    CC_PROGMEM = "[H";
const char VT_KEY_NM_END_F[]     CC_PROGMEM = "[F";
const char VT_KEY_NM_F1[]        CC_PROGMEM = "[11~";
const char VT_KEY_NM_F2[]        CC_PROGMEM = "[12~";
const char VT_KEY_NM_F3[]        CC_PROGMEM = "[13~";
const char VT_KEY_NM_F4[]        CC_PROGMEM = "[14~";
const char VT_KEY_NM_F5[]        CC_PROGMEM = "[15~";
const char VT_KEY_NM_F6[]        CC_PROGMEM = "[17~";
const char VT_KEY_NM_F7[]        CC_PROGMEM = "[18~";
const char VT_KEY_NM_F8[]        CC_PROGMEM = "[19~";
const char VT_KEY_NM_F9[]        CC_PROGMEM = "[20~";
const char VT_KEY_NM_F10[]       CC_PROGMEM = "[21~";
const char VT_KEY_NM_F11[]       CC_PROGMEM = "[23~";
const char VT_KEY_NM_F12[]       CC_PROGMEM = "[24~";
const char VT_KEY_CTL_UP[]       CC_PROGMEM = "OA";
const char VT_KEY_CTL_DOWN[]     CC_PROGMEM = "OB";
const char VT_KEY_CTL_RIGHT[]    CC_PROGMEM = "OC";
const char VT_KEY_CTL_LEFT[]     CC_PROGMEM = "OD";
const char VT_KEY_ALT_UP[]       CC_PROGMEM = "\e[A";
const char VT_KEY_ALT_DOWN[]     CC_PROGMEM = "\e[B";
const char VT_KEY_ALT_RIGHT[]    CC_PROGMEM = "\e[C";
const char VT_KEY_ALT_LEFT[]     CC_PROGMEM = "\e[D";
const char VT_KEY_EXT_UP[]       CC_PROGMEM = "\eOA";
const char VT_KEY_EXT_DOWN[]     CC_PROGMEM = "\eOB";
const char VT_KEY_EXT_RIGHT[]    CC_PROGMEM = "\eOC";
const char VT_KEY_EXT_LEFT[]     CC_PROGMEM = "\eOD";
const char VT_KEY_CTL_HOME[]     CC_PROGMEM = "OH";
const char VT_KEY_CTL_END[]      CC_PROGMEM = "OF";
const char VT_KEY_NM_CTL_UP[]    CC_PROGMEM = "[1;5A";
const char VT_KEY_NM_CTL_DOWN[]  CC_PROGMEM = "[1;5B";
const char VT_KEY_NM_CTL_RIGHT[] CC_PROGMEM = "[1;5C";
const char VT_KEY_NM_CTL_LEFT[]  CC_PROGMEM = "[1;5D";
const char VT_KEY_NM_ALT_UP[]    CC_PROGMEM = "[1;3A";
const char VT_KEY_NM_ALT_DOWN[]  CC_PROGMEM = "[1;3B";
const char VT_KEY_NM_ALT_RIGHT[] CC_PROGMEM = "[1;3C";
const char VT_KEY_NM_ALT_LEFT[]  CC_PROGMEM = "[1;3D";

struct vt_key_map
{
  const char * vt_seq;
  rune16_t key;
};

const struct vt_key_map vt_key_mappings[] CC_PROGMEM =
{
  { VT_KEY_NM_UP       , KEY_UP },
  { VT_KEY_NM_DOWN     , KEY_DOWN },
  { VT_KEY_NM_RIGHT    , KEY_RIGHT },
  { VT_KEY_NM_LEFT     , KEY_LEFT },
  { VT_KEY_NM_HOME     , KEY_HOME },
  { VT_KEY_NM_HOME_H   , KEY_HOME },
  { VT_KEY_NM_IC       , KEY_IC },
  { VT_KEY_NM_DC       , KEY_DC }, // 0x7F
  { VT_KEY_NM_END      , KEY_END },
  { VT_KEY_NM_END_F    , KEY_END },
  { VT_KEY_CTL_HOME    , KEY_HOME },
  { VT_KEY_CTL_END     , KEY_END },
  { VT_KEY_NM_PPAGE    , KEY_PPAGE },
  { VT_KEY_NM_NPAGE    , KEY_NPAGE },
  { VT_KEY_NM_CENTER   , KEY_CR },  // ?
  { VT_KEY_NM_BTAB     , KEY_BTAB },
  { VT_KEY_NM_F1       , KEY_F(1) },
  { VT_KEY_NM_F2       , KEY_F(2) },
  { VT_KEY_NM_F3       , KEY_F(3) },
  { VT_KEY_NM_F4       , KEY_F(4) },
  { VT_KEY_NM_F5       , KEY_F(5) },
  { VT_KEY_NM_F6       , KEY_F(6) },
  { VT_KEY_NM_F7       , KEY_F(7) },
  { VT_KEY_NM_F8       , KEY_F(8) },
  { VT_KEY_NM_F9       , KEY_F(9) },
  { VT_KEY_NM_F10      , KEY_F(10) },
  { VT_KEY_NM_F11      , KEY_F(11) },
  { VT_KEY_NM_F12      , KEY_F(12) },
  { VT_KEY_CTL_UP      , KEY_SB },
  { VT_KEY_CTL_DOWN    , KEY_SF },
  { VT_KEY_CTL_RIGHT   , KEY_NW },
  { VT_KEY_CTL_LEFT    , KEY_PW },
  { VT_KEY_ALT_UP      , KEY_MLU },
  { VT_KEY_ALT_DOWN    , KEY_MLD },
  { VT_KEY_ALT_RIGHT   , KEY_MLR },
  { VT_KEY_ALT_LEFT    , KEY_MLL },
  { VT_KEY_EXT_UP      , KEY_MLU },
  { VT_KEY_EXT_DOWN    , KEY_MLD },
  { VT_KEY_EXT_RIGHT   , KEY_MLR },
  { VT_KEY_EXT_LEFT    , KEY_MLL },
  { VT_KEY_NM_CTL_UP   , KEY_SB },
  { VT_KEY_NM_CTL_DOWN , KEY_SF },
  { VT_KEY_NM_CTL_RIGHT, KEY_NW },
  { VT_KEY_NM_CTL_LEFT , KEY_PW },
  { VT_KEY_NM_ALT_UP   , KEY_MLU },
  { VT_KEY_NM_ALT_DOWN , KEY_MLD },
  { VT_KEY_NM_ALT_RIGHT, KEY_MLR },
  { VT_KEY_NM_ALT_LEFT , KEY_MLL },
};

// VT400 output sequence codes.
const char VT_SEQ_CSI[]                 CC_PROGMEM = "\e[";     // code introducer
const char VT_SEQ_CLEAR[]               CC_PROGMEM = "\e[2J";   // clear screen
const char VT_SEQ_CLRTOBOT[]            CC_PROGMEM = "\e[J";    // clear to bottom
const char VT_SEQ_CLRTOEOL[]            CC_PROGMEM = "\e[K";    // clear to end of line
const char VT_SEQ_DELCH[]               CC_PROGMEM = "\e[P";    // delete character
const char VT_SEQ_NEXTLINE[]            CC_PROGMEM = "\eE";     // goto next line (scroll up at end of scrolling region)
const char VT_SEQ_INSERTLINE[]          CC_PROGMEM = "\e[L";    // insert line
const char VT_SEQ_DELETELINE[]          CC_PROGMEM = "\e[M";    // delete line
const char VT_SEQ_ATTRSET[]             CC_PROGMEM = "\e[0";    // set attributes, e.g. "\e[0;7;1m"
const char VT_SEQ_ATTRSET_REVERSE[]     CC_PROGMEM = ";7";      // reverse
const char VT_SEQ_ATTRSET_UNDERLINE[]   CC_PROGMEM = ";4";      // underline
const char VT_SEQ_ATTRSET_BLINK[]       CC_PROGMEM = ";5";      // blink
const char VT_SEQ_ATTRSET_BOLD[]        CC_PROGMEM = ";1";      // bold
const char VT_SEQ_ATTRSET_DIM[]         CC_PROGMEM = ";2";      // dim
const char VT_SEQ_ATTRSET_FCOLOR[]      CC_PROGMEM = ";3";      // forground color
const char VT_SEQ_ATTRSET_BCOLOR[]      CC_PROGMEM = ";4";      // background color
const char VT_SEQ_INSERT_MODE[]         CC_PROGMEM = "\e[4h";   // set insert mode
const char VT_SEQ_REPLACE_MODE[]        CC_PROGMEM = "\e[4l";   // set replace mode
const char VT_SEQ_RESET_SCRREG[]        CC_PROGMEM = "\e[r";    // reset scrolling region
const char VT_SEQ_LOAD_G1[]             CC_PROGMEM = "\e)0";    // load G1 character set
const char VT_SEQ_CURSOR_VIS[]          CC_PROGMEM = "\e[?25";  // set cursor visible/not visible

#ifdef __AVR__
#define GET_VT_SEQ(idx) (const char *)pgm_read_word(&vt_key_mappings[idx].vt_seq)
#define GET_KEY(idx) (rune16_t)pgm_read_word(&vt_key_mappings[idx].key)
#else
#define GET_VT_SEQ(idx) vt_key_mappings[idx].vt_seq
#define GET_KEY(idx) vt_key_mappings[idx].key
#ifndef strcmp_P
#define strcmp_P strcmp
#endif
#endif

#define VT_IF_ESCAPE_END(c) ((c >= 'A' && c <= 'D') || c == 'G' || c == 'Z' || c == '~' || c == 'H' || c == 'F')

const size_t vt_key_mappings_size = (sizeof(vt_key_mappings) / sizeof(struct vt_key_map));

int8_t vt_esc_add16(char * buffer, uint8_t * idx, const rune16_t ch)
{
    if (ch > 0x7F)
        return ERR_ARG_INVALID;

    if(!(*idx < (VT_ESCAPE_BUFLEN - 1)))
        return ERR_BUF_OVERFLOW;

    buffer[(*idx)++] = ch;
    if (VT_IF_ESCAPE_END(ch))
    {
        buffer[(*idx)] = '\0';
        return ERR_SUCCESS; // SUCCESS
    }
    else
    {
        return ERR_YIELDING; // WAITING
    }
}

rune16_t vt_esc_match16(const char * buffer, const uint8_t len)
{
    for(int idx = 0; idx < vt_key_mappings_size; ++idx)
    {
        if (strcmp_P(buffer, GET_VT_SEQ(idx)) == 0)
        {
            return GET_KEY(idx);
        }
    }

    return UTF8_DECODE_ERROR;
}

rune16_t vt_esc_symbol16(const rune16_t rune)
{
    switch(rune)
    {
        case KEY_TAB: return ACS_TAB;
        case KEY_ENTER: return ACS_ENTER;
        case KEY_PAUSE: return ACS_PAUSE;
        case KEY_BACKSPACE: return ACS_BACKSPACE;
        case KEY_ESCAPE: return ASC_ESC;

        case KEY_UP: return ACS_CURSOR_UP;
        case KEY_DOWN: return ACS_CURSOR_DOWN;
        case KEY_RIGHT: return ACS_CURSOR_RIGHT;
        case KEY_LEFT: return ACS_CURSOR_LEFT;

        case KEY_HOME: return ACS_HOME;
        case KEY_DC: return ACS_DELETE;
        case KEY_IC: return ACS_INSERT;
        case KEY_NPAGE: return ACS_PGDN;
        case KEY_PPAGE: return ACS_PGUP;
        case KEY_END: return ACS_END;
        case KEY_BTAB: return ACS_BTAB;

        case KEY_SF: return ACS_CURSOR_DOWN;
        case KEY_SB: return ACS_CURSOR_UP;
        case KEY_NW: return ACS_CURSOR_RIGHT;
        case KEY_PW: return ACS_CURSOR_LEFT;

        case KEY_MLU: return ACS_CURSOR_UP;
        case KEY_MLD: return ACS_CURSOR_DOWN;
        case KEY_MLR: return ACS_CURSOR_RIGHT;
        case KEY_MLL: return ACS_CURSOR_LEFT;

        default: return rune;
    };
}


