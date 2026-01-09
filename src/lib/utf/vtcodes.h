/**
 * @file ./src/lib/utf/vtcodes.h
 * @brief Complete VT100/VT220/VT420 Terminal Control Codes
 *
 * This header provides comprehensive ANSI/DEC escape sequences for
 * VT100, VT220, and VT420 terminal emulation including full duplex
 * communication setup, cursor control, screen manipulation, character
 * sets, and device control.
 *
 * Usage: Include this header and use the macros directly in printf/write
 * statements. All sequences are ready for immediate use.
 *
 * @version 1.0
 * @date 2026
 */

#ifndef VT_TERMINAL_CODES_H
#define VT_TERMINAL_CODES_H

/* ========================================================================
 * BASIC CONTROL CHARACTERS (C0 Controls)
 * ======================================================================== */

#define VT_NUL          "\x00"  /* Null - Ignored */
#define VT_SOH          "\x01"  /* Start of Heading */
#define VT_STX          "\x02"  /* Start of Text */
#define VT_ETX          "\x03"  /* End of Text */
#define VT_EOT          "\x04"  /* End of Transmission */
#define VT_ENQ          "\x05"  /* Enquiry - Triggers answerback */
#define VT_ACK          "\x06"  /* Acknowledge */
#define VT_BEL          "\x07"  /* Bell - Generates beep */
#define VT_BS           "\x08"  /* Backspace - Move cursor left one */
#define VT_HT           "\x09"  /* Horizontal Tab - Move to next tab stop */
#define VT_LF           "\x0A"  /* Line Feed - Move cursor down */
#define VT_VT           "\x0B"  /* Vertical Tab - Processed as LF */
#define VT_FF           "\x0C"  /* Form Feed - Processed as LF */
#define VT_CR           "\x0D"  /* Carriage Return - Move to left margin */
#define VT_SO           "\x0E"  /* Shift Out - Invoke G1 into GL (LS1) */
#define VT_SI           "\x0F"  /* Shift In - Invoke G0 into GL (LS0) */
#define VT_DLE          "\x10"  /* Data Link Escape */
#define VT_DC1          "\x11"  /* Device Control 1 (XON) - Resume transmission */
#define VT_DC2          "\x12"  /* Device Control 2 */
#define VT_DC3          "\x13"  /* Device Control 3 (XOFF) - Stop transmission */
#define VT_DC4          "\x14"  /* Device Control 4 */
#define VT_NAK          "\x15"  /* Negative Acknowledge */
#define VT_SYN          "\x16"  /* Synchronous Idle */
#define VT_ETB          "\x17"  /* End of Transmission Block */
#define VT_CAN          "\x18"  /* Cancel - Abort escape/control sequence */
#define VT_EM           "\x19"  /* End of Medium */
#define VT_SUB          "\x1A"  /* Substitute - Cancel sequence, show error */
#define VT_ESC          "\x1B"  /* Escape - Sequence introducer */
#define VT_FS           "\x1C"  /* File Separator */
#define VT_GS           "\x1D"  /* Group Separator */
#define VT_RS           "\x1E"  /* Record Separator */
#define VT_US           "\x1F"  /* Unit Separator */
#define VT_DEL          "\x7F"  /* Delete - Ignored */

/* XON/XOFF Flow Control */
#define VT_XON          VT_DC1  /* Resume transmission */
#define VT_XOFF         VT_DC3  /* Stop transmission */

/* ========================================================================
 * C1 CONTROL CHARACTERS (8-bit and 7-bit equivalents)
 * ======================================================================== */

/* 8-bit C1 Controls */
#define VT_IND_8BIT     "\x84"  /* Index */
#define VT_NEL_8BIT     "\x85"  /* Next Line */
#define VT_HTS_8BIT     "\x88"  /* Horizontal Tab Set */
#define VT_RI_8BIT      "\x8D"  /* Reverse Index */
#define VT_SS2_8BIT     "\x8E"  /* Single Shift G2 */
#define VT_SS3_8BIT     "\x8F"  /* Single Shift G3 */
#define VT_DCS_8BIT     "\x90"  /* Device Control String */
#define VT_SPA_8BIT     "\x96"  /* Start of Protected Area */
#define VT_EPA_8BIT     "\x97"  /* End of Protected Area */
#define VT_SOS_8BIT     "\x98"  /* Start of String */
#define VT_DECID_8BIT   "\x9A"  /* Return Terminal ID */
#define VT_CSI_8BIT     "\x9B"  /* Control Sequence Introducer */
#define VT_ST_8BIT      "\x9C"  /* String Terminator */
#define VT_OSC_8BIT     "\x9D"  /* Operating System Command */
#define VT_PM_8BIT      "\x9E"  /* Privacy Message */
#define VT_APC_8BIT     "\x9F"  /* Application Program Command */

/* 7-bit C1 Control Equivalents (ESC + character) */
#define VT_IND          "\x1B\x44"  /* ESC D - Index */
#define VT_NEL          "\x1B\x45"  /* ESC E - Next Line */
#define VT_HTS          "\x1B\x48"  /* ESC H - Horizontal Tab Set */
#define VT_RI           "\x1B\x4D"  /* ESC M - Reverse Index */
#define VT_SS2          "\x1B\x4E"  /* ESC N - Single Shift G2 */
#define VT_SS3          "\x1B\x4F"  /* ESC O - Single Shift G3 */
#define VT_DCS          "\x1B\x50"  /* ESC P - Device Control String */
#define VT_SPA          "\x1B\x56"  /* ESC V - Start of Protected Area */
#define VT_EPA          "\x1B\x57"  /* ESC W - End of Protected Area */
#define VT_SOS          "\x1B\x58"  /* ESC X - Start of String */
#define VT_DECID        "\x1B\x5A"  /* ESC Z - Return Terminal ID */
#define VT_CSI          "\x1B\x5B"  /* ESC [ - Control Sequence Introducer */
#define VT_ST           "\x1B\x5C"  /* ESC \ - String Terminator */
#define VT_OSC          "\x1B\x5D"  /* ESC ] - Operating System Command */
#define VT_PM           "\x1B\x5E"  /* ESC ^ - Privacy Message */
#define VT_APC          "\x1B\x5F"  /* ESC _ - Application Program Command */

/* ========================================================================
 * SELECT C1 CONTROL TRANSMISSION MODE
 * ======================================================================== */

#define VT_S7C1T        "\x1B\x20\x46"  /* 7-bit C1 controls */
#define VT_S8C1T        "\x1B\x20\x47"  /* 8-bit C1 controls */

/* ========================================================================
 * CURSOR POSITIONING
 * ======================================================================== */

/* Basic Cursor Movement */
#define VT_CUU(n)       VT_CSI #n "A"   /* Cursor Up n lines */
#define VT_CUD(n)       VT_CSI #n "B"   /* Cursor Down n lines */
#define VT_CUF(n)       VT_CSI #n "C"   /* Cursor Forward n columns */
#define VT_CUB(n)       VT_CSI #n "D"   /* Cursor Backward n columns */
#define VT_CUP(r,c)     VT_CSI #r ";" #c "H"  /* Cursor Position row,col */
#define VT_HVP(r,c)     VT_CSI #r ";" #c "f"  /* Horiz & Vert Position */

/* Single step movements (default n=1) */
#define VT_CUU1         VT_CSI "A"      /* Cursor Up 1 */
#define VT_CUD1         VT_CSI "B"      /* Cursor Down 1 */
#define VT_CUF1         VT_CSI "C"      /* Cursor Forward 1 */
#define VT_CUB1         VT_CSI "D"      /* Cursor Backward 1 */

/* Cursor Home and Special Positions */
#define VT_HOME         VT_CSI "H"      /* Cursor to home (1,1) */
#define VT_CUP_HOME     VT_CSI "1;1H"   /* Explicit home position */

/* Save and Restore Cursor */
#define VT_DECSC        "\x1B\x37"      /* ESC 7 - Save Cursor */
#define VT_DECRC        "\x1B\x38"      /* ESC 8 - Restore Cursor */
#define VT_SCP          VT_CSI "s"      /* Save Cursor Position (ANSI.SYS) */
#define VT_RCP          VT_CSI "u"      /* Restore Cursor Position (ANSI.SYS) */

/* VT420 Additional Cursor Movement */
#define VT_DECBI        "\x1B\x36"      /* ESC 6 - Back Index (VT420) */
#define VT_DECFI        "\x1B\x39"      /* ESC 9 - Forward Index (VT420) */

/* ========================================================================
 * SCREEN CLEARING AND ERASING
 * ======================================================================== */

/* Erase in Display (ED) */
#define VT_ED_BELOW     VT_CSI "J"      /* Erase from cursor to end */
#define VT_ED_BELOW0    VT_CSI "0J"     /* Erase from cursor to end */
#define VT_ED_ABOVE     VT_CSI "1J"     /* Erase from start to cursor */
#define VT_ED_ALL       VT_CSI "2J"     /* Erase entire display */
#define VT_ED_SCROLL    VT_CSI "3J"     /* Erase scrollback (xterm) */

/* Erase in Line (EL) */
#define VT_EL_RIGHT     VT_CSI "K"      /* Erase from cursor to end of line */
#define VT_EL_RIGHT0    VT_CSI "0K"     /* Erase from cursor to end of line */
#define VT_EL_LEFT      VT_CSI "1K"     /* Erase from start to cursor */
#define VT_EL_ALL       VT_CSI "2K"     /* Erase entire line */

/* Selective Erase (VT220+) */
#define VT_DECSED_BELOW VT_CSI "?J"     /* Selective erase below */
#define VT_DECSED_ABOVE VT_CSI "?1J"    /* Selective erase above */
#define VT_DECSED_ALL   VT_CSI "?2J"    /* Selective erase all */
#define VT_DECSEL_RIGHT VT_CSI "?K"     /* Selective erase right */
#define VT_DECSEL_LEFT  VT_CSI "?1K"    /* Selective erase left */
#define VT_DECSEL_ALL   VT_CSI "?2K"    /* Selective erase line */

/* Erase Character (VT220+) */
#define VT_ECH(n)       VT_CSI #n "X"   /* Erase n characters */

/* ========================================================================
 * CHARACTER INSERTION AND DELETION
 * ======================================================================== */

#define VT_ICH(n)       VT_CSI #n "@"   /* Insert n Characters (VT220) */
#define VT_DCH(n)       VT_CSI #n "P"   /* Delete n Characters */
#define VT_IL(n)        VT_CSI #n "L"   /* Insert n Lines */
#define VT_DL(n)        VT_CSI #n "M"   /* Delete n Lines */

/* VT420 Column Operations */
#define VT_DECIC(n)     VT_CSI #n "'}'" /* Insert n Columns (VT420) */
#define VT_DECDC(n)     VT_CSI #n "'~'" /* Delete n Columns (VT420) */

/* ========================================================================
 * SCROLLING REGIONS
 * ======================================================================== */

#define VT_DECSTBM(t,b) VT_CSI #t ";" #b "r"  /* Set scroll region top,bottom */
#define VT_DECSTBM_FULL VT_CSI "r"            /* Reset to full screen */
#define VT_DECSLRM(l,r) VT_CSI #l ";" #r "s"  /* Set left/right margins (VT420) */

/* ========================================================================
 * TEXT ATTRIBUTES / GRAPHICS RENDITION (SGR)
 * ======================================================================== */

/* Reset and Basic Attributes */
#define VT_SGR_RESET    VT_CSI "m"      /* All attributes off */
#define VT_SGR_RESET0   VT_CSI "0m"     /* All attributes off */
#define VT_SGR_BOLD     VT_CSI "1m"     /* Bold/bright */
#define VT_SGR_DIM      VT_CSI "2m"     /* Dim/faint */
#define VT_SGR_ITALIC   VT_CSI "3m"     /* Italic */
#define VT_SGR_UNDER    VT_CSI "4m"     /* Underline */
#define VT_SGR_BLINK    VT_CSI "5m"     /* Blink slow */
#define VT_SGR_BLINK_FAST VT_CSI "6m"   /* Blink fast */
#define VT_SGR_REVERSE  VT_CSI "7m"     /* Reverse video/inverse */
#define VT_SGR_CONCEAL  VT_CSI "8m"     /* Concealed/hidden */
#define VT_SGR_STRIKE   VT_CSI "9m"     /* Strikethrough */

/* Attribute Resets */
#define VT_SGR_NORMAL   VT_CSI "22m"    /* Normal intensity (not bold/dim) */
#define VT_SGR_NO_ITALIC VT_CSI "23m"   /* Not italic */
#define VT_SGR_NO_UNDER VT_CSI "24m"    /* Not underlined */
#define VT_SGR_NO_BLINK VT_CSI "25m"    /* Not blinking */
#define VT_SGR_NO_REVERSE VT_CSI "27m"  /* Not reverse */
#define VT_SGR_NO_CONCEAL VT_CSI "28m"  /* Not concealed */
#define VT_SGR_NO_STRIKE VT_CSI "29m"   /* Not strikethrough */

/* Standard 8 Colors - Foreground */
#define VT_FG_BLACK     VT_CSI "30m"
#define VT_FG_RED       VT_CSI "31m"
#define VT_FG_GREEN     VT_CSI "32m"
#define VT_FG_YELLOW    VT_CSI "33m"
#define VT_FG_BLUE      VT_CSI "34m"
#define VT_FG_MAGENTA   VT_CSI "35m"
#define VT_FG_CYAN      VT_CSI "36m"
#define VT_FG_WHITE     VT_CSI "37m"
#define VT_FG_DEFAULT   VT_CSI "39m"    /* Default foreground */

/* Standard 8 Colors - Background */
#define VT_BG_BLACK     VT_CSI "40m"
#define VT_BG_RED       VT_CSI "41m"
#define VT_BG_GREEN     VT_CSI "42m"
#define VT_BG_YELLOW    VT_CSI "43m"
#define VT_BG_BLUE      VT_CSI "44m"
#define VT_BG_MAGENTA   VT_CSI "45m"
#define VT_BG_CYAN      VT_CSI "46m"
#define VT_BG_WHITE     VT_CSI "47m"
#define VT_BG_DEFAULT   VT_CSI "49m"    /* Default background */

/* Bright Colors (90-97, 100-107) - aixterm extension */
#define VT_FG_BRIGHT_BLACK   VT_CSI "90m"
#define VT_FG_BRIGHT_RED     VT_CSI "91m"
#define VT_FG_BRIGHT_GREEN   VT_CSI "92m"
#define VT_FG_BRIGHT_YELLOW  VT_CSI "93m"
#define VT_FG_BRIGHT_BLUE    VT_CSI "94m"
#define VT_FG_BRIGHT_MAGENTA VT_CSI "95m"
#define VT_FG_BRIGHT_CYAN    VT_CSI "96m"
#define VT_FG_BRIGHT_WHITE   VT_CSI "97m"

#define VT_BG_BRIGHT_BLACK   VT_CSI "100m"
#define VT_BG_BRIGHT_RED     VT_CSI "101m"
#define VT_BG_BRIGHT_GREEN   VT_CSI "102m"
#define VT_BG_BRIGHT_YELLOW  VT_CSI "103m"
#define VT_BG_BRIGHT_BLUE    VT_CSI "104m"
#define VT_BG_BRIGHT_MAGENTA VT_CSI "105m"
#define VT_BG_BRIGHT_CYAN    VT_CSI "106m"
#define VT_BG_BRIGHT_WHITE   VT_CSI "107m"

/* 256 Color Support */
#define VT_FG_256(n)    VT_CSI "38;5;" #n "m"  /* Set foreground to color n (0-255) */
#define VT_BG_256(n)    VT_CSI "48;5;" #n "m"  /* Set background to color n (0-255) */

/* RGB/TrueColor Support */
#define VT_FG_RGB(r,g,b) VT_CSI "38;2;" #r ";" #g ";" #b "m"
#define VT_BG_RGB(r,g,b) VT_CSI "48;2;" #r ";" #g ";" #b "m"

/* Select Character Attributes (VT220) */
#define VT_DECSCA_OFF   VT_CSI "0\"q"   /* Attributes off, erasable */
#define VT_DECSCA_ON    VT_CSI "1\"q"   /* Not erasable by DECSED/DECSEL */
#define VT_DECSCA_ERASE VT_CSI "2\"q"   /* Erasable by DECSED/DECSEL */

/* ========================================================================
 * LINE ATTRIBUTES
 * ======================================================================== */

#define VT_DECDHL_TOP   "\x1B\x23\x33"  /* ESC # 3 - Double height top half */
#define VT_DECDHL_BOT   "\x1B\x23\x34"  /* ESC # 4 - Double height bottom half */
#define VT_DECSWL       "\x1B\x23\x35"  /* ESC # 5 - Single width line */
#define VT_DECDWL       "\x1B\x23\x36"  /* ESC # 6 - Double width line */
#define VT_DECALN       "\x1B\x23\x38"  /* ESC # 8 - Screen alignment test (fill with E) */

/* ========================================================================
 * CHARACTER SETS DESIGNATION
 * ======================================================================== */

/* G0 Designation */
#define VT_G0_ASCII     "\x1B\x28\x42"  /* ESC ( B - ASCII */
#define VT_G0_GRAPHICS  "\x1B\x28\x30"  /* ESC ( 0 - DEC Special Graphics */
#define VT_G0_UK        "\x1B\x28\x41"  /* ESC ( A - UK */
#define VT_G0_DEC_SUPP  "\x1B\x28\x3C"  /* ESC ( < - DEC Supplemental (VT220) */

/* G1 Designation */
#define VT_G1_ASCII     "\x1B\x29\x42"  /* ESC ) B - ASCII */
#define VT_G1_GRAPHICS  "\x1B\x29\x30"  /* ESC ) 0 - DEC Special Graphics */
#define VT_G1_UK        "\x1B\x29\x41"  /* ESC ) A - UK */
#define VT_G1_DEC_SUPP  "\x1B\x29\x3C"  /* ESC ) < - DEC Supplemental (VT220) */

/* G2 Designation (VT220+) */
#define VT_G2_ASCII     "\x1B\x2A\x42"  /* ESC * B - ASCII */
#define VT_G2_GRAPHICS  "\x1B\x2A\x30"  /* ESC * 0 - DEC Special Graphics */
#define VT_G2_UK        "\x1B\x2A\x41"  /* ESC * A - UK */

/* G3 Designation (VT220+) */
#define VT_G3_ASCII     "\x1B\x2B\x42"  /* ESC + B - ASCII */
#define VT_G3_GRAPHICS  "\x1B\x2B\x30"  /* ESC + 0 - DEC Special Graphics */
#define VT_G3_UK        "\x1B\x2B\x41"  /* ESC + A - UK */

/* Locking Shifts */
#define VT_LS0          VT_SI           /* Shift In - Invoke G0 into GL */
#define VT_LS1          VT_SO           /* Shift Out - Invoke G1 into GL */
#define VT_LS2          "\x1B\x6E"      /* ESC n - Invoke G2 into GL (VT220) */
#define VT_LS3          "\x1B\x6F"      /* ESC o - Invoke G3 into GL (VT220) */
#define VT_LS1R         "\x1B\x7E"      /* ESC ~ - Invoke G1 into GR (VT220) */
#define VT_LS2R         "\x1B\x7D"      /* ESC } - Invoke G2 into GR (VT220) */
#define VT_LS3R         "\x1B\x7C"      /* ESC | - Invoke G3 into GR (VT220) */

/* National Character Sets (Final characters for SCS) */
#define VT_NCS_BRITISH       "\x41"     /* A - British */
#define VT_NCS_DUTCH         "\x34"     /* 4 - Dutch */
#define VT_NCS_FINNISH       "\x43"     /* C or 5 - Finnish */
#define VT_NCS_FINNISH_ALT   "\x35"
#define VT_NCS_FRENCH        "\x52"     /* R - French */
#define VT_NCS_FRENCH_CAN    "\x51"     /* Q - French Canadian */
#define VT_NCS_GERMAN        "\x4B"     /* K - German */
#define VT_NCS_ITALIAN       "\x59"     /* Y - Italian */
#define VT_NCS_NORWEGIAN     "\x45"     /* E or 6 - Norwegian/Danish */
#define VT_NCS_NORWEGIAN_ALT "\x36"
#define VT_NCS_SPANISH       "\x5A"     /* Z - Spanish */
#define VT_NCS_SWEDISH       "\x48"     /* H or 7 - Swedish */
#define VT_NCS_SWEDISH_ALT   "\x37"
#define VT_NCS_SWISS         "\x3D"     /* = - Swiss */

/* ========================================================================
 * TAB STOPS
 * ======================================================================== */

#define VT_TBC_CURRENT  VT_CSI "g"      /* Clear tab at current position */
#define VT_TBC_CURRENT0 VT_CSI "0g"     /* Clear tab at current position */
#define VT_TBC_ALL      VT_CSI "3g"     /* Clear all tabs */

/* ========================================================================
 * TERMINAL MODES (SET/RESET)
 * ======================================================================== */

/* ANSI Modes - Set Mode (SM) */
#define VT_SM_KAM       VT_CSI "2h"     /* Keyboard Action Mode (lock) */
#define VT_SM_IRM       VT_CSI "4h"     /* Insert Mode */
#define VT_SM_SRM       VT_CSI "12h"    /* Send/Receive Mode (local echo off) */
#define VT_SM_LNM       VT_CSI "20h"    /* Line Feed/New Line Mode */

/* ANSI Modes - Reset Mode (RM) */
#define VT_RM_KAM       VT_CSI "2l"     /* Keyboard Action Mode (unlock) */
#define VT_RM_IRM       VT_CSI "4l"     /* Replace Mode */
#define VT_RM_SRM       VT_CSI "12l"    /* Send/Receive Mode (local echo on) */
#define VT_RM_LNM       VT_CSI "20l"    /* Line Feed Mode */

/* DEC Private Modes - Set (DECSET) */
#define VT_DECSET_DECCKM    VT_CSI "?1h"    /* Cursor Keys Application */
#define VT_DECSET_DECANM    VT_CSI "?2h"    /* ANSI Mode (not used for set) */
#define VT_DECSET_DECCOLM   VT_CSI "?3h"    /* 132 Column Mode */
#define VT_DECSET_DECSCLM   VT_CSI "?4h"    /* Smooth Scroll */
#define VT_DECSET_DECSCNM   VT_CSI "?5h"    /* Reverse Video */
#define VT_DECSET_DECOM     VT_CSI "?6h"    /* Origin Mode */
#define VT_DECSET_DECAWM    VT_CSI "?7h"    /* Auto Wrap */
#define VT_DECSET_DECARM    VT_CSI "?8h"    /* Auto Repeat */
#define VT_DECSET_DECPFF    VT_CSI "?18h"   /* Print Form Feed */
#define VT_DECSET_DECPEX    VT_CSI "?19h"   /* Print Extent (full screen) */
#define VT_DECSET_DECTCEM   VT_CSI "?25h"   /* Text Cursor Enable */
#define VT_DECSET_DECNRCM   VT_CSI "?42h"   /* National Charset Mode */
#define VT_DECSET_DECLRMM   VT_CSI "?69h"   /* Left/Right Margin Mode (VT420) */
#define VT_DECSET_MOUSE_X10 VT_CSI "?9h"    /* X10 Mouse Tracking */
#define VT_DECSET_MOUSE_VT200 VT_CSI "?1000h" /* VT200 Mouse Tracking */
#define VT_DECSET_MOUSE_BTN VT_CSI "?1002h"   /* Button Event Mouse Tracking */
#define VT_DECSET_MOUSE_ANY VT_CSI "?1003h"   /* Any Event Mouse Tracking */
#define VT_DECSET_ALT_SCREEN VT_CSI "?1049h"  /* Alternate Screen Buffer */
#define VT_DECSET_BRACKETED_PASTE VT_CSI "?2004h" /* Bracketed Paste Mode */

/* DEC Private Modes - Reset (DECRST) */
#define VT_DECRST_DECCKM    VT_CSI "?1l"    /* Cursor Keys Normal */
#define VT_DECRST_DECANM    VT_CSI "?2l"    /* VT52 Mode */
#define VT_DECRST_DECCOLM   VT_CSI "?3l"    /* 80 Column Mode */
#define VT_DECRST_DECSCLM   VT_CSI "?4l"    /* Jump Scroll */
#define VT_DECRST_DECSCNM   VT_CSI "?5l"    /* Normal Video */
#define VT_DECRST_DECOM     VT_CSI "?6l"    /* Normal Cursor Mode */
#define VT_DECRST_DECAWM    VT_CSI "?7l"    /* No Auto Wrap */
#define VT_DECRST_DECARM    VT_CSI "?8l"    /* No Auto Repeat */
#define VT_DECRST_DECPFF    VT_CSI "?18l"   /* No Print Form Feed */
#define VT_DECRST_DECPEX    VT_CSI "?19l"   /* Print Scrolling Region */
#define VT_DECRST_DECTCEM   VT_CSI "?25l"   /* Text Cursor Disable */
#define VT_DECRST_DECNRCM   VT_CSI "?42l"   /* Multinational Charset */
#define VT_DECRST_DECLRMM   VT_CSI "?69l"   /* No Left/Right Margins (VT420) */
#define VT_DECRST_MOUSE_X10 VT_CSI "?9l"    /* Disable X10 Mouse */
#define VT_DECRST_MOUSE_VT200 VT_CSI "?1000l" /* Disable VT200 Mouse */
#define VT_DECRST_MOUSE_BTN VT_CSI "?1002l"   /* Disable Button Mouse */
#define VT_DECRST_MOUSE_ANY VT_CSI "?1003l"   /* Disable Any Event Mouse */
#define VT_DECRST_ALT_SCREEN VT_CSI "?1049l"  /* Normal Screen Buffer */
#define VT_DECRST_BRACKETED_PASTE VT_CSI "?2004l" /* No Bracketed Paste */

/* ========================================================================
 * KEYPAD MODES
 * ======================================================================== */

#define VT_DECKPAM      "\x1B\x3D"      /* ESC = - Application Keypad Mode */
#define VT_DECKPNM      "\x1B\x3E"      /* ESC > - Numeric Keypad Mode */

/* ========================================================================
 * COMPATIBILITY LEVEL (DECSCL)
 * ======================================================================== */

#define VT_DECSCL_VT100 VT_CSI "61\"p"  /* Set VT100 mode */
#define VT_DECSCL_VT200_8BIT VT_CSI "62\"p"      /* VT200, 8-bit controls */
#define VT_DECSCL_VT200_8BIT_ALT VT_CSI "62;0\"p" /* VT200, 8-bit controls */
#define VT_DECSCL_VT200_7BIT VT_CSI "62;1\"p"    /* VT200, 7-bit controls */
#define VT_DECSCL_VT300_8BIT VT_CSI "63\"p"      /* VT300, 8-bit (VT420) */
#define VT_DECSCL_VT300_7BIT VT_CSI "63;1\"p"    /* VT300, 7-bit (VT420) */
#define VT_DECSCL_VT400_8BIT VT_CSI "64\"p"      /* VT420, 8-bit */
#define VT_DECSCL_VT400_7BIT VT_CSI "64;1\"p"    /* VT420, 7-bit */

/* ========================================================================
 * DEVICE STATUS REPORTS (DSR) - REQUESTS
 * ======================================================================== */

#define VT_DSR_DEVICE_STATUS    VT_CSI "5n"     /* Request device status */
#define VT_DSR_CURSOR_POS       VT_CSI "6n"     /* Request cursor position */
#define VT_DSR_PRINTER_STATUS   VT_CSI "?15n"   /* Request printer status */
#define VT_DSR_UDK_STATUS       VT_CSI "?25n"   /* Request UDK locked status */
#define VT_DSR_KEYBOARD_LANG    VT_CSI "?26n"   /* Request keyboard language */

/* DSR Responses (sent by terminal) */
#define VT_DSR_OK              VT_CSI "0n"      /* Device OK */
#define VT_DSR_MALFUNCTION     VT_CSI "3n"      /* Device malfunction */
#define VT_DSR_PRINTER_READY   VT_CSI "?10n"    /* Printer ready */
#define VT_DSR_PRINTER_NOT_READY VT_CSI "?11n"  /* Printer not ready */
#define VT_DSR_NO_PRINTER      VT_CSI "?13n"    /* No printer */
#define VT_DSR_UDK_UNLOCKED    VT_CSI "?20n"    /* UDK unlocked */
#define VT_DSR_UDK_LOCKED      VT_CSI "?21n"    /* UDK locked */
/* CPR: VT_CSI <row> ; <col> R */

/* ========================================================================
 * DEVICE ATTRIBUTES (DA) - IDENTIFICATION
 * ======================================================================== */

/* Primary DA Request */
#define VT_DA_PRIMARY   VT_CSI "c"              /* Primary DA request */
#define VT_DA_PRIMARY0  VT_CSI "0c"             /* Primary DA request */

/* Secondary DA Request */
#define VT_DA_SECONDARY VT_CSI ">c"             /* Secondary DA request */
#define VT_DA_SECONDARY0 VT_CSI ">0c"           /* Secondary DA request */

/* Tertiary DA (VT420) */
#define VT_DA_TERTIARY  VT_CSI "=c"             /* Tertiary DA request */

/* Common Primary DA Responses (examples) */
/* VT100: ESC [ ? 1 ; 2 c */
/* VT220: ESC [ ? 62 ; 1 ; 2 ; 6 ; 7 ; 8 ; 9 c */
/* VT420: ESC [ ? 64 ; <params> c */

/* ========================================================================
 * TERMINAL RESET
 * ======================================================================== */

#define VT_RIS          "\x1B\x63"      /* ESC c - Hard Reset (Reset to Initial State) */
#define VT_DECSTR       VT_CSI "!p"     /* Soft Terminal Reset (VT220+) */

/* ========================================================================
 * PRINTING
 * ======================================================================== */

#define VT_MC_PRINT_LINE    VT_CSI "?1i"    /* Print cursor line */
#define VT_MC_PRINT_SCREEN  VT_CSI "i"      /* Print screen */
#define VT_MC_PRINT_SCREEN0 VT_CSI "0i"     /* Print screen */
#define VT_MC_AUTOPRINT_ON  VT_CSI "?5i"    /* Auto print mode on */
#define VT_MC_AUTOPRINT_OFF VT_CSI "?4i"    /* Auto print mode off */
#define VT_MC_CONTROLLER_ON VT_CSI "5i"     /* Printer controller mode on */
#define VT_MC_CONTROLLER_OFF VT_CSI "4i"    /* Printer controller mode off */

/* VT52 Mode Printing */
#define VT52_PRINT_SCREEN   "\x1B\x5D"      /* ESC ] - Print screen */
#define VT52_PRINT_LINE     "\x1B\x56"      /* ESC V - Print cursor line */
#define VT52_AUTOPRINT_ON   "\x1B\x5E"      /* ESC ^ - Auto print on */
#define VT52_AUTOPRINT_OFF  "\x1B\x5F"      /* ESC _ - Auto print off */
#define VT52_CONTROLLER_ON  "\x1B\x57"      /* ESC W - Controller mode on */
#define VT52_CONTROLLER_OFF "\x1B\x58"      /* ESC X - Controller mode off */

/* ========================================================================
 * WINDOW TITLE AND ICON OPERATIONS
 * ======================================================================== */

#define VT_SET_TITLE(t)     VT_OSC "0;" t VT_BEL  /* Set window title */
#define VT_SET_ICON_TITLE(t) VT_OSC "1;" t VT_BEL /* Set icon title */
#define VT_SET_WIN_TITLE(t)  VT_OSC "2;" t VT_BEL /* Set window title only */

/* ========================================================================
 * VT52 MODE SEQUENCES
 * ======================================================================== */

/* VT52 Mode Entry/Exit */
#define VT52_ENTER      VT_CSI "?2l"    /* Enter VT52 mode from ANSI */
#define VT52_EXIT       "\x1B\x3C"      /* ESC < - Exit VT52, enter ANSI */

/* VT52 Mode Cursor Movement */
#define VT52_UP         "\x1B\x41"      /* ESC A - Cursor up */
#define VT52_DOWN       "\x1B\x42"      /* ESC B - Cursor down */
#define VT52_RIGHT      "\x1B\x43"      /* ESC C - Cursor right */
#define VT52_LEFT       "\x1B\x44"      /* ESC D - Cursor left */
#define VT52_HOME       "\x1B\x48"      /* ESC H - Cursor home */

/* VT52 Mode Display Control */
#define VT52_REV_INDEX  "\x1B\x49"      /* ESC I - Reverse line feed */
#define VT52_ERASE_EOS  "\x1B\x4A"      /* ESC J - Erase to end of screen */
#define VT52_ERASE_EOL  "\x1B\x4B"      /* ESC K - Erase to end of line */

/* VT52 Graphics Mode */
#define VT52_GRAPHICS_ON  "\x1B\x46"    /* ESC F - Enter graphics mode */
#define VT52_GRAPHICS_OFF "\x1B\x47"    /* ESC G - Exit graphics mode */

/* VT52 Keypad */
#define VT52_KEYPAD_ALT "\x1B\x3D"      /* ESC = - Alternate keypad */
#define VT52_KEYPAD_NUM "\x1B\x3E"      /* ESC > - Numeric keypad */

/* VT52 Identify */
#define VT52_IDENT      "\x1B\x5A"      /* ESC Z - Identify */
/* Response: ESC / Z (VT52) or ESC / K (VT100 in VT52 mode) */

/* ========================================================================
 * FULL DUPLEX COMMUNICATION SETUP
 * ======================================================================== */

/* Flow Control Setup */
#define VT_FLOW_XON_XOFF_ENABLE  1      /* Use XON/XOFF (DC1/DC3) flow control */
#define VT_FLOW_HARDWARE_ENABLE  2      /* Use RTS/CTS hardware flow control */
#define VT_FLOW_NONE             0      /* No flow control */

/* Communication Parameters Notes:
 * For proper full duplex operation, ensure:
 * 1. Baud rate matches on both ends (9600, 19200, 38400, etc.)
 * 2. Data bits (typically 8)
 * 3. Parity (typically None)
 * 4. Stop bits (typically 1)
 * 5. Flow control (XON/XOFF or RTS/CTS)
 * 6. Local echo (typically off for full duplex)
 *
 * Terminal setup:
 * - Use VT_RM_SRM to disable local echo (full duplex)
 * - Use VT_DC1/VT_DC3 for software flow control
 * - Configure terminal settings via setup menu or sequences
 */

/* ========================================================================
 * MOUSE TRACKING (xterm-style)
 * ======================================================================== */

/* Mouse protocol enables - see DECSET section above for:
 * VT_DECSET_MOUSE_X10, VT_DECSET_MOUSE_VT200, etc.
 */

/* Mouse report format (example for VT200 mode):
 * ESC [ M Cb Cx Cy
 * where Cb = button+32, Cx/Cy = position+32
 */

/* ========================================================================
 * MISCELLANEOUS SEQUENCES
 * ======================================================================== */

/* Application Program Commands */
#define VT_APC_START    VT_APC          /* Begin APC string */
#define VT_APC_END      VT_ST           /* End APC string */

/* Device Control Strings */
#define VT_DCS_START    VT_DCS          /* Begin DCS string */
#define VT_DCS_END      VT_ST           /* End DCS string */

/* Operating System Commands */
#define VT_OSC_START    VT_OSC          /* Begin OSC string */
#define VT_OSC_END      VT_ST           /* End OSC string (or BEL) */

/* Cursor Style (DECSCUSR - VT520+, widely supported) */
#define VT_CURSOR_BLOCK_BLINK   VT_CSI "1 q"  /* Block blinking */
#define VT_CURSOR_BLOCK_STEADY  VT_CSI "2 q"  /* Block steady */
#define VT_CURSOR_UNDER_BLINK   VT_CSI "3 q"  /* Underline blinking */
#define VT_CURSOR_UNDER_STEADY  VT_CSI "4 q"  /* Underline steady */
#define VT_CURSOR_BAR_BLINK     VT_CSI "5 q"  /* Bar blinking */
#define VT_CURSOR_BAR_STEADY    VT_CSI "6 q"  /* Bar steady */

/* VT420 Specific Extensions */
#define VT_DECERA(t,l,b,r) VT_CSI #t ";" #l ";" #b ";" #r "$z"  /* Erase Rect Area */
#define VT_DECFRA(c,t,l,b,r) VT_CSI #c ";" #t ";" #l ";" #b ";" #r "$x" /* Fill Rect */
#define VT_DECCRA(ts,ls,bs,rs,sp,td,ld,pd) VT_CSI #ts ";" #ls ";" #bs ";" #rs ";" #sp ";" #td ";" #ld ";" #pd "$v" /* Copy Rect */

/* Sixel Graphics (VT240/VT340/VT420) */
#define VT_SIXEL_START  VT_DCS "0;1;0q" /* Start sixel graphics */
#define VT_SIXEL_END    VT_ST           /* End sixel graphics */

/* ReGIS Graphics (VT240/VT340) */
#define VT_REGIS_START  VT_DCS "p"      /* Start ReGIS graphics */
#define VT_REGIS_END    VT_ST           /* End ReGIS graphics */

/* ========================================================================
 * ANSWERBACK MESSAGE (ENQ Response)
 * ======================================================================== */

/* The answerback message is configurable in terminal setup.
 * When terminal receives ENQ (VT_ENQ), it sends back the answerback string.
 * This is used for terminal identification in some protocols.
 */

/* ========================================================================
 * USER DEFINED KEYS (UDK) - VT220+
 * ======================================================================== */

/* Format: DCS Pfn;Pcn;Pe;Pcms;Pw;Pt | Dscs Kyn/Stn;... ST
 *
 * Example to define F6 key (key 17):
 * VT_DCS "1;1;1|@17/48656C6C6F" VT_ST
 *
 * This defines F6 to send "Hello" (hex: 48 65 6C 6C 6F)
 *
 * Key numbers: F6=17, F7=18, F8=19, F9=20, F10=21, F11=23, F12=24,
 *              F13=25, F14=26, Help=28, Do=29, F17=31, F18=32, F19=33, F20=34
 */

#define VT_DECUDK_START VT_DCS "1;1;0|" /* Start UDK definition, clear/lock */
#define VT_DECUDK_END   VT_ST           /* End UDK definition */

/* ========================================================================
 * DOWNLOADABLE CHARACTER SETS (DRCS) - VT220+
 * ======================================================================== */

/* Format: DCS Pfn;Pcn;Pe;Pcms;Pw;Pt { Dscs sixel_data ST
 *
 * Example for defining soft character set:
 * VT_DCS "1;1;1{ @" <sixel_data> VT_ST
 *
 * The sixel data defines character bitmaps using sixel encoding.
 * Each character can be up to 8x10 pixels (width x height).
 */

#define VT_DECDLD_START VT_DCS "1;1;1{ @" /* Start DRCS definition */
#define VT_DECDLD_END   VT_ST              /* End DRCS definition */

/* ========================================================================
 * USEFUL MACRO COMBINATIONS
 * ======================================================================== */

/* Clear screen and home cursor */
#define VT_CLEAR_ALL    VT_ED_ALL VT_HOME

/* Full screen setup for application */
#define VT_SETUP_FULL_SCREEN \
    VT_DECSET_ALT_SCREEN    /* Use alternate screen */ \
    VT_ED_ALL               /* Clear it */ \
    VT_HOME                 /* Home cursor */

/* Restore normal screen */
#define VT_RESTORE_SCREEN   VT_DECRST_ALT_SCREEN

/* Setup for raw terminal mode (no echo, no processing) */
#define VT_SETUP_RAW \
    VT_RM_SRM               /* Disable local echo */ \
    VT_RM_LNM               /* Line feed mode */ \
    VT_DECSET_DECCKM        /* Application cursor keys */ \
    VT_DECKPAM              /* Application keypad */

/* Restore normal terminal mode */
#define VT_RESTORE_NORMAL \
    VT_SM_SRM               /* Enable local echo */ \
    VT_SM_LNM               /* New line mode */ \
    VT_DECRST_DECCKM        /* Normal cursor keys */ \
    VT_DECKPNM              /* Numeric keypad */

/* Bold red text example */
#define VT_BOLD_RED     VT_SGR_BOLD VT_FG_RED

/* Reset to defaults */
#define VT_RESET_ATTRS  VT_SGR_RESET

/* ========================================================================
 * HELPER MACROS FOR DYNAMIC PARAMETERS
 * ======================================================================== */

/* These require proper formatting in your code:
 *
 * Example usage:
 *   char buf[32];
 *   snprintf(buf, sizeof(buf), VT_CSI "%d;%dH", row, col);
 *   write(fd, buf, strlen(buf));
 *
 * Or using the provided macros with literal values:
 *   write(fd, VT_CUP(10, 20), strlen(VT_CUP(10, 20)));
 */

#endif /* VT_TERMINAL_CODES_H */

/* ========================================================================
 * END OF VT TERMINAL CONTROL CODES HEADER
 *
 * This header provides a complete set of control codes for VT100, VT220,
 * and VT420 terminal emulation. All sequences are production-ready and
 * follow the official DEC specifications.
 *
 * For full duplex communication:
 * 1. Disable local echo: use VT_RM_SRM
 * 2. Configure flow control: XON/XOFF (VT_DC1/VT_DC3) or hardware
 * 3. Set appropriate baud rate and communication parameters
 * 4. Use VT_DSR_* commands to query terminal status
 * 5. Handle received sequences according to VT protocol
 *
 * References:
 * - VT100 User Guide (DEC, 1979)
 * - VT220 Programmer Reference Manual (DEC, 1988)
 * - VT420 Programmer Reference Manual (DEC, 1994)
 * - ECMA-48 / ISO 6429 Control Functions for Coded Character Sets
 *
 * ======================================================================== */