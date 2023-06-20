#include "vt100.h"

// VT400 INPUT CODES
CC_CONST_PSTR(VT_KEY_NM_UP        ,"[A");
CC_CONST_PSTR(VT_KEY_NM_DOWN      ,"[B");
CC_CONST_PSTR(VT_KEY_NM_RIGHT     ,"[C");
CC_CONST_PSTR(VT_KEY_NM_LEFT      ,"[D");   
CC_CONST_PSTR(VT_KEY_NM_HOME      ,"[1~");
CC_CONST_PSTR(VT_KEY_NM_IC        ,"[2~");
CC_CONST_PSTR(VT_KEY_NM_DC        ,"[3~");
CC_CONST_PSTR(VT_KEY_NM_END       ,"[4~");  
CC_CONST_PSTR(VT_KEY_NM_PPAGE     ,"[5~");
CC_CONST_PSTR(VT_KEY_NM_NPAGE     ,"[6~");  
CC_CONST_PSTR(VT_KEY_NM_CENTER    ,"[G");
CC_CONST_PSTR(VT_KEY_NM_BTAB      ,"[Z");   
CC_CONST_PSTR(VT_KEY_NM_F1        ,"[11~");
CC_CONST_PSTR(VT_KEY_NM_F2        ,"[12~");
CC_CONST_PSTR(VT_KEY_NM_F3        ,"[13~");
CC_CONST_PSTR(VT_KEY_NM_F4        ,"[14~");
CC_CONST_PSTR(VT_KEY_NM_F5        ,"[15~");
CC_CONST_PSTR(VT_KEY_NM_F6        ,"[17~");
CC_CONST_PSTR(VT_KEY_NM_F7        ,"[18~");
CC_CONST_PSTR(VT_KEY_NM_F8        ,"[19~");
CC_CONST_PSTR(VT_KEY_NM_F9        ,"[20~");
CC_CONST_PSTR(VT_KEY_NM_F10       ,"[21~");
CC_CONST_PSTR(VT_KEY_NM_F11       ,"[23~");
CC_CONST_PSTR(VT_KEY_NM_F12       ,"[24~");    
CC_CONST_PSTR(VT_KEY_CTL_UP       ,"OA");
CC_CONST_PSTR(VT_KEY_CTL_DOWN     ,"OB");
CC_CONST_PSTR(VT_KEY_CTL_RIGHT    ,"OC");
CC_CONST_PSTR(VT_KEY_CTL_LEFT     ,"OD");  
CC_CONST_PSTR(VT_KEY_ALT_UP       ,"\e[A");
CC_CONST_PSTR(VT_KEY_ALT_DOWN     ,"\e[B");
CC_CONST_PSTR(VT_KEY_ALT_RIGHT    ,"\e[C");
CC_CONST_PSTR(VT_KEY_ALT_LEFT     ,"\e[D");    
CC_CONST_PSTR(VT_KEY_EXT_UP       ,"\eOA");
CC_CONST_PSTR(VT_KEY_EXT_DOWN     ,"\eOB");
CC_CONST_PSTR(VT_KEY_EXT_RIGHT    ,"\eOC");
CC_CONST_PSTR(VT_KEY_EXT_LEFT     ,"\eOD");

struct vt_key_map
{
  const char * vt_seq;
  rune16_t key;
};

CC_CONST_PSTRUCT_ARR(vt_key_map, vt_key_mappings) =
{
  { VT_KEY_NM_UP    , KEY_UP },
  { VT_KEY_NM_DOWN  , KEY_DOWN },
  { VT_KEY_NM_RIGHT , KEY_RIGHT },
  { VT_KEY_NM_LEFT  , KEY_LEFT },
  { VT_KEY_NM_HOME  , KEY_HOME },  
  { VT_KEY_NM_IC    , KEY_IC },  
  { VT_KEY_NM_DC    , KEY_DC }, // 0x7F 
  { VT_KEY_NM_END   , KEY_END },  
  { VT_KEY_NM_PPAGE , KEY_PPAGE },  
  { VT_KEY_NM_NPAGE , KEY_NPAGE },  
  { VT_KEY_NM_CENTER, KEY_CR },  // ?
  { VT_KEY_NM_BTAB  , KEY_BTAB },  
  { VT_KEY_NM_F1    , KEY_F(1) },  
  { VT_KEY_NM_F2    , KEY_F(2) },  
  { VT_KEY_NM_F3    , KEY_F(3) },  
  { VT_KEY_NM_F4    , KEY_F(4) },  
  { VT_KEY_NM_F5    , KEY_F(5) },  
  { VT_KEY_NM_F6    , KEY_F(6) },  
  { VT_KEY_NM_F7    , KEY_F(7) },  
  { VT_KEY_NM_F8    , KEY_F(8) },  
  { VT_KEY_NM_F9    , KEY_F(9) },  
  { VT_KEY_NM_F10   , KEY_F(10) },  
  { VT_KEY_NM_F11   , KEY_F(11) },  
  { VT_KEY_NM_F12   , KEY_F(12) },  
  { VT_KEY_CTL_UP   , KEY_SB },  
  { VT_KEY_CTL_DOWN , KEY_SF },  
  { VT_KEY_CTL_RIGHT, KEY_NW },  
  { VT_KEY_CTL_LEFT , KEY_PW },  
  { VT_KEY_ALT_UP   , KEY_MLU },  
  { VT_KEY_ALT_DOWN , KEY_MLD },  
  { VT_KEY_ALT_RIGHT, KEY_CR }, // ?
  { VT_KEY_ALT_LEFT , KEY_CR }, // ?
};

// VT400 output sequence codes.
CC_CONST_PSTR(VT_SEQ_CSI                 ,"\e[");     // code introducer
CC_CONST_PSTR(VT_SEQ_CLEAR               ,"\e[2J");   // clear screen
CC_CONST_PSTR(VT_SEQ_CLRTOBOT            ,"\e[J");    // clear to bottom
CC_CONST_PSTR(VT_SEQ_CLRTOEOL            ,"\e[K");    // clear to end of line
CC_CONST_PSTR(VT_SEQ_DELCH               ,"\e[P");    // delete character
CC_CONST_PSTR(VT_SEQ_NEXTLINE            ,"\eE");     // goto next line (scroll up at end of scrolling region)
CC_CONST_PSTR(VT_SEQ_INSERTLINE          ,"\e[L");    // insert line
CC_CONST_PSTR(VT_SEQ_DELETELINE          ,"\e[M");    // delete line
CC_CONST_PSTR(VT_SEQ_ATTRSET             ,"\e[0");    // set attributes, e.g. "\e[0;7;1m"
CC_CONST_PSTR(VT_SEQ_ATTRSET_REVERSE     ,";7");      // reverse
CC_CONST_PSTR(VT_SEQ_ATTRSET_UNDERLINE   ,";4");      // underline
CC_CONST_PSTR(VT_SEQ_ATTRSET_BLINK       ,";5");      // blink
CC_CONST_PSTR(VT_SEQ_ATTRSET_BOLD        ,";1");      // bold
CC_CONST_PSTR(VT_SEQ_ATTRSET_DIM         ,";2");      // dim
CC_CONST_PSTR(VT_SEQ_ATTRSET_FCOLOR      ,";3");      // forground color
CC_CONST_PSTR(VT_SEQ_ATTRSET_BCOLOR      ,";4");      // background color
CC_CONST_PSTR(VT_SEQ_INSERT_MODE         ,"\e[4h");   // set insert mode
CC_CONST_PSTR(VT_SEQ_REPLACE_MODE        ,"\e[4l");   // set replace mode
CC_CONST_PSTR(VT_SEQ_RESET_SCRREG        ,"\e[r");    // reset scrolling region
CC_CONST_PSTR(VT_SEQ_LOAD_G1             ,"\e)0");    // load G1 character set
CC_CONST_PSTR(VT_SEQ_CURSOR_VIS          ,"\e[?25");  // set cursor visible/not visible

#define VT_IF_ESCAPE_END(c) ((c >= 'A' && c <= 'D') || c == 'G' || c == 'Z' || c == '~')

const size_t vt_key_mappings_size = (sizeof(vt_key_mappings) / sizeof(struct vt_key_map));

int8_t vt_escape_add(char * buffer, uint8_t * idx, const rune16_t ch)
{
    if (ch > 0x7F)
        return VT_ERR_INVALID_INPUT;

    if(!(*idx < (VT_ESCAPE_BUFLEN - 1)))
        return VT_ERR_BUFFER_OVERFLOW;
    
    if (VT_IF_ESCAPE_END(ch))
    {
        buffer[(*idx)++] = ch; ;
        buffer[(*idx)++] = '\0';
        return 0; // SUCCESS
    }
    else
    {
        buffer[(*idx)++] = ch;
        return 1; // WAITING
    }
}

rune16_t vt_escape_match(const char * buffer, const uint8_t len)
{
    uint8_t size = min(len, VT_ESCAPE_BUFLEN);
    for(int idx = 0; idx < vt_key_mappings_size; ++idx)
    {
        if (strncmp_P(buffer, vt_key_mappings[idx].vt_seq, size))
        {
            return vt_key_mappings[idx].key;
        }       
    }

    return UTF8_DECODE_ERROR;
}

rune16_t vt_escape_symbol(const rune16_t rune)
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

        // case KEY_DL:
        // case KEY_IL:
        // case KEY_SF:
        // case KEY_SB:

        // case KEY_NW:
        // case KEY_PW:
        // case KEY_MLU:
        // case KEY_MLD: 
        //     return ASC_UNKNOWN;

        default: return rune;
    };
}
