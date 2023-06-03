#ifndef __VT100_H__
#define __VT100_H__

#include <protoduino.h>
#include "utf8.h"

// key codes for rune16_t between 0x80-0x9F (size == 32)
// https://www.gnu.org/software/guile-ncurses/manual/html_node/Getting-characters-from-the-keyboard.html
#define KEY_TAB                 '\t'            // TAB key
#define KEY_CR                  '\r'            // RETURN key
#define KEY_BACKSPACE           '\b'            // Backspace key
#define KEY_ENTER               KEY_CR          // RETURN key
#define KEY_PAUSE               0x1A            // PAUSE key
#define KEY_ESCAPE              0x1B            // ESCAPE
#define KEY_DELETE              0x7F            // Delete

#define KEY_C0                  0x80
#define KEY_C(n)                (KEY_C0+(n))
#define KEY_UP                  KEY_C(0)        // Up arrow key
#define KEY_DOWN                KEY_C(1)        // Down arrow key
#define KEY_RIGHT               KEY_C(2)        // Right arrow key
#define KEY_LEFT                KEY_C(3)        // Left arrow key

#define KEY_HOME                KEY_C(4)        // Home key
#define KEY_DC                  KEY_DELETE      // Delete character key
#define KEY_IC                  KEY_C(5)        // Ins char/toggle ins mode key
#define KEY_NPAGE               KEY_C(6)        // Next-page key
#define KEY_PPAGE               KEY_C(7)        // Previous-page key
#define KEY_END                 KEY_C(8)        // End key
#define KEY_BTAB                KEY_C(9)       // Back tab key

#define KEY_DL                  KEY_C(10)       // Delete Line (Shift + Delete)
#define KEY_IL                  KEY_C(11)       // Insert line (Shift + Insert)

#define KEY_SF                  KEY_C(12)       // Scroll 1 line forward (CTRL + Down)
#define KEY_SB                  KEY_C(13)       // Scroll 1 line backwards (CTRL + Up)

#define KEY_NW                  KEY_C(14)       // Move word next (CTRL + Right)
#define KEY_PW                  KEY_C(15)       // Move word previous (CTRL + Left)

#define KEY_MLU                 KEY_C(16)       // Move line 1 up (ALT + Up)
#define KEY_MLD                 KEY_C(17)       // Move line 1 down (ALT + Down)

#define KEY_F0                  KEY_C(18)       // Function key F0
#define KEY_F(n)                (KEY_F0+(n)-1)  // Space for additional 12 function keys

//
//
//
// DEC Special Graphics Character Set
// http://justsolve.archiveteam.org/wiki/DEC_Special_Graphics_Character_Set


#define ACS_DIAMOND             0x25C6  // DEC graphic 0x60: diamond
#define ACS_CKBOARD             0x2592  // DEC graphic 0x61: checker board
#define ACS_HT                  0x2409  // DEC graphic 0x62: horizontal tab
#define ACS_FF                  0x240C  // DEC graphic 0x63: form Feed
#define ACS_CR                  0x240D  // DEC graphic 0x64: carriage Return
#define ACS_LF                  0x240A  // DEC graphic 0x65: line Feed
#define ACS_DEGREE              0x00B0  // DEC graphic 0x66: degree sign
#define ACS_PLMINUS             0x00B1  // DEC graphic 0x67: plus/minus
#define ACS_NL                  0x2424  // DEC graphic 0x68: new line
#define ACS_VT                  0x240B  // DEC graphic 0x69: vertical tab

#define ACS_LRCORNER            0x2518  // DEC graphic 0x6a: lower right corner
#define ACS_URCORNER            0x2510  // DEC graphic 0x6b: upper right corner
#define ACS_ULCORNER            0x250C  // DEC graphic 0x6c: upper left corner
#define ACS_LLCORNER            0x2514  // DEC graphic 0x6d: lower left corner
#define ACS_PLUS                0x253C  // DEC graphic 0x6e: crossing lines

#define ACS_S1                  0x23BA  // DEC graphic 0x6f: scan line 1
#define ACS_S3                  0x23BB  // DEC graphic 0x70: scan line 3
#define ACS_S5                  0x2500  // DEC graphic 0x71: scan line 5
#define ACS_HLINE               ACS_S5  // DEC graphic 0x71: horizontal line
#define ACS_S7                  0x23BC  // DEC graphic 0x72: scan line 7
#define ACS_S9                  0x23BD  // DEC graphic 0x73: scan line 9

#define ACS_LTEE                0x251C  // DEC graphic 0x74: left tee
#define ACS_RTEE                0x2524  // DEC graphic 0x75: right tee
#define ACS_BTEE                0x2534  // DEC graphic 0x76: bottom tee
#define ACS_TTEE                0x252C  // DEC graphic 0x77: top tee
#define ACS_VLINE               0x2502  // DEC graphic 0x78: vertical line

#define ACS_LEQUAL              0x2264  // DEC graphic 0x79: less/equal
#define ACS_GEQUAL              0x2265  // DEC graphic 0x7a: greater/equal
#define ACS_PI                  0x03C0  // DEC graphic 0x7b: Pi
#define ACS_NEQUAL              0x2260  // DEC graphic 0x7c: not equal
#define ACS_STERLING            0x00A3  // DEC graphic 0x7d: uk pound sign
#define ACS_BULLET              0x00B7  // DEC graphic 0x7e: bullet


// rune symbol definitions
// http://xahlee.info/comp/unicode_computing_symbols.html
// https://markentier.tech/posts/2021/04/windows-shortcut-key-symbol/

#define ASC_UNKNOWN             0x003F  // '?'
#define ACS_C0                  0x2726  // '✦' U+2726
#define ACS_CTRL                0x2732  // NA -> '✲' U+2732
#define ACS_ALT                 0x2387  // NA -> '⎇' U+2387

#define ACS_BACKSPACE           0x232B  // 8 -> '⌫' U+232B
#define ACS_TAB                 0x2B7E  // 9 -> '⭾' U+2B7E
#define ACS_BTAB                0x21C6  // NA -> '⇆' U+21C6
#define ACS_ENTER               0x23CE  // 13 -> '↵' U+23CE
#define ACS_PAUSE               0x2389  // 26 -> '⎉' U+2389

// #define ASC_ESC              0x238B  // 27 -> '⎋' U+238B
#define ASC_ESC                 0x241B  // 27 -> '␛' U+241B

#define ACS_HOME                0x2912 // NA -> '⤒' U+2912
#define ACS_INSERT              0x2380 // '⎀' U+2380
#define ACS_DELETE              0x2326 // 127 -> '⌦' U+2326
#define ACS_END                 0x2913 // NA -> '⤓' U+2913
#define ACS_PGUP                0x21DE // NA -> '⇞' U+21DE
#define ACS_PGDN                0x21DF // NA -> '⇟' -> U+21DF

#define ACS_CURSOR_UP           0x25B2 // '▲' U+25B2
#define ACS_CURSOR_DOWN         0x25BC // '▼' U+25BC
#define ACS_CURSOR_RIGHT        0x21E5 // '▶' U+21E5
#define ACS_CURSOR_LEFT         0x25C0 // '◀' U+25C0

///
///
///

#define VT_ERR_INVALID_INPUT -1
#define VT_ERR_BUFFER_OVERFLOW -2
#define VT_ERR_KEY_NOT_FOUND -3

int8_t vt_escape_add(char * buffer, uint8_t * idx, const rune16_t ch);

rune16_t vt_escape_match(const char * buffer, const uint8_t len);

rune16_t vt_escape_symbol(rune16_t rune);

// VT400 output sequence codes.
// CC_EXPORT_CONST_PSTR(VT_SEQ_CSI);
// CC_EXPORT_CONST_PSTR(VT_SEQ_CLEAR);
// CC_EXPORT_CONST_PSTR(VT_SEQ_CLRTOBOT);
// CC_EXPORT_CONST_PSTR(VT_SEQ_CLRTOEOL);
// CC_EXPORT_CONST_PSTR(VT_SEQ_DELCH);
// CC_EXPORT_CONST_PSTR(VT_SEQ_NEXTLINE);
// CC_EXPORT_CONST_PSTR(VT_SEQ_INSERTLINE);
// CC_EXPORT_CONST_PSTR(VT_SEQ_DELETELINE);
// CC_EXPORT_CONST_PSTR(VT_SEQ_ATTRSET);
// CC_EXPORT_CONST_PSTR(VT_SEQ_ATTRSET_REVERSE);
// CC_EXPORT_CONST_PSTR(VT_SEQ_ATTRSET_UNDERLINE);
// CC_EXPORT_CONST_PSTR(VT_SEQ_ATTRSET_BLINK);
// CC_EXPORT_CONST_PSTR(VT_SEQ_ATTRSET_BOLD);
// CC_EXPORT_CONST_PSTR(VT_SEQ_ATTRSET_DIM);
// CC_EXPORT_CONST_PSTR(VT_SEQ_ATTRSET_FCOLOR);
// CC_EXPORT_CONST_PSTR(VT_SEQ_ATTRSET_BCOLOR);
// CC_EXPORT_CONST_PSTR(VT_SEQ_INSERT_MODE);
// CC_EXPORT_CONST_PSTR(VT_SEQ_REPLACE_MODE);
// CC_EXPORT_CONST_PSTR(VT_SEQ_RESET_SCRREG);
// CC_EXPORT_CONST_PSTR(VT_SEQ_LOAD_G1);
// CC_EXPORT_CONST_PSTR(VT_SEQ_CURSOR_VIS);

#define VT_SEQ_END_ATTRSET 'm'

#endif
