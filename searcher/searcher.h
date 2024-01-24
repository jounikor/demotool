/*
 * v0.1 (c) 2024 Jouni 'Mr.Spiv' Korhonen
 * 
 * =======================================================================
 *
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <http://unlicense.org/>
 */
#ifndef _SEARCHER_H_INCLUDED
#define _SEARCHER_H_INCLUDED

#include <exec/types.h>

/* versioning */
#define MAJOR       3
#define MINOR       0

#define TSTR(s) #s
#define MSTR(s) TSTR(s)

/* Aligh display modes with the relevant indices */
#define MODE_STD    0x00000001
#define MODE_HAM    0x00000002
#define MODE_DUALPF 0x00000004
#define MODE_PF2PRI 0x00000008
#define MODE_HIRES  0x00000010

typedef struct {
    UWORD pal[32+3];
    LONG bpl_ptr[6];
    LONG pal_ptr;
    LONG max_chipmem;
    WORD mod_even;      // in bytes
    WORD mod_odd;       // in bytes
    WORD scr_width;     // in pixels
    WORD scr_height;    // in pixels
    BYTE num_bpl;
    UBYTE pf1shft;
    UBYTE pf2shft;
    UBYTE display;
    UBYTE priority;
    UBYTE save_as;
    UBYTE bpl_lock;
    BYTE rgb_idx;
    ULONG flags;
    LONG ioerr;
    UBYTE status;
    UBYTE padding;
} cop_cfg_t;

#define TEXT_AREA0_X    10
#define TEXT_AREA0_Y    3+9

#define TEXT_AREA1_X    206
#define TEXT_AREA1_Y    3+7
#define TEXT_AREA1_LEN  18
#define TEXT_SCREEN     0x00000001
#define TEXT_PF1WIDTH   0x00000002
#define TEXT_PF2WIDTH   0x00000004
#define TEXT_HEIGHT     0x00000008
#define TEXT_COMMENT    0x00000010
#define TEXT_ADDRESS    0x00000020

#define TEXT_AREA2_X    358
#define TEXT_AREA2_Y    3+7
#define TEXT_AREA2_LEN  13
#define TEXT_MODEVEN    0x00000100
#define TEXT_MODODD     0x00000200
#define TEXT_DISPLAY    0x00000400
#define TEXT_PRIORITY   0x00000800
#define TEXT_SAVEAS     0x00001000
#define TEXT_RGB        0x00002000
#define TEXT_STATUS     0x00004000  /* Status line */

#define TEXT_AREA3_X    470
#define TEXT_AREA3_Y    3+7
#define TEXT_AREA3_LEN  19
#define TEXT_BPLANES    0x00010000
#define TEXT_RESO       0x00020000
#define TEXT_BPLLOCKS   0x00040000
#define TEXT_BPLADDRS   0x00080000

#define RGB_CHANGE      0x00100000


/* limits */
#define MAX_LORES_SCREEN_WIDTH  320+48
#define MAX_HIRES_SCREEN_WIDTH  640+96
#define MAX_BPLANE_WIDTH        4096
#define MAX_MODULO              1000

/* mask to determine if the copper list needs to be updated */
#define TEXT_COPPER_MASK    TEXT_MODEVEN| \
                            TEXT_MODODD|TEXT_DISPLAY|TEXT_PRIORITY| \
                            TEXT_BPLANES|TEXT_RESO|TEXT_BPLADDRS  | \
                            RGB_CHANGE
#define IDX_PRIO_PF1    0
#define IDX_PRIO_PF2    1
#define IDX_PRIO_MAX    1

#define IDX_DISP_STD    0
#define IDX_DISP_HAM    1
#define IDX_DISP_DPF    2
#define IDX_DISP_MAX    2

#define IDX_SAVE_IFF    0
#define IDX_SAVE_RAW    1
#define IDX_SAVE_PAL    2
#define IDX_SAVE_MAX    2

#define IDX_STATUS_VERSION  0
#define IDX_STATUS_PALETTE  1
#define IDX_STATUS_NOSAVE   2
#define IDX_STATUS_SAVEOK   3
#define IDX_STATUS_OK       4
#define IDX_STATUS_NORAM    5
#define IDX_STATUS_FAULT    6
#define IDX_STATUS_EXISTS   7
#define IDX_STATUS_NOTFOUND 8
#define IDX_STATUS_MAX      8


/* key codes */
#define KEY_ESC     0x45
#define KEY_F1      0x50
#define KEY_F2      0x51
#define KEY_F3      0x52
#define KEY_F4      0x53
#define KEY_F5      0x54
#define KEY_F6      0x55
#define KEY_F7      0x56
#define KEY_F8      0x57
#define KEY_F9      0x58
#define KEY_F10     0x59

#define KEY_1       0x01
#define KEY_2       0x02
#define KEY_3       0x03
#define KEY_4       0x04
#define KEY_5       0x05
#define KEY_6       0x06
#define KEY_7       0x07
#define KEY_8       0x08
#define KEY_9       0x09
#define KEY_0       0x0a
#define KEY_MINUS   0x0b
#define KEY_EQUAL   0x0c
#define KEY_BSLASH  0x0d
#define KEY_BSPACE  0x41
#define KEY_P       0x19
#define KEY_D       0x22
#define KEY_H       0x25
#define KEY_L       0x28
#define KEY_X       0x32
#define KEY_C       0x33
#define KEY_V       0x34
#define KEY_B       0x35
#define KEY_N       0x36
#define KEY_M       0x37
#define KEY_COMMA   0x38
#define KEY_FSTOP   0x39
#define KEY_SLASH   0x3a

#define KEY_LSHFT   0x60
#define KEY_RSHFT   0x61
#define KEY_LALT    0x64
#define KEY_RALT    0x65
#define KEY_LAMI    0x66
#define KEY_RAMI    0x67

#define KEY_TAB     0x42
#define KEY_DEL     0x46
#define KEY_HELP    0x5f
#define KEY_RETURN  0x44
#define KEY_ENTER   0x43

#define KEY_UP      0x4c
#define KEY_DOWN    0x4d
#define KEY_LEFT    0x4f
#define KEY_RIGHT   0x4e

#define KEY_LBRA    0x1a
#define KEY_RBRA    0x1b


/* numpad */
#define KEY_NP_FS       0x3c
#define KEY_NP_ENTER    0x43
#define KEY_NP_0        0x0f
#define KEY_NP_9        0x3f
#define KEY_NP_8        0x3e
#define KEY_NP_7        0x3d
#define KEY_NP_6        0x2f
#define KEY_NP_5        0x2e
#define KEY_NP_4        0x2d
#define KEY_NP_3        0x1f
#define KEY_NP_2        0x1e
#define KEY_NP_1        0x1d
#define KEY_NP_MINUS    0x4a

/*
 */
#define MAX_FILENAME    128
#define GAD_STR_W       144
#define GAD_STR_H       8

/*
 */




/* screenn planning .. */
#if 0

128

206                 358            470
0         1         2         3    |    4         5
012345678901234567890123456789012345678901234567890123

SCREEN WIDTH  0000  RGB  #00 $000  BIT PLANES  4 HIRES
PFIELD1 WIDTH 0000  MOD EVEN 0000  0 ON  * $100000 PF1
PFIELD2 WIDTH 0000  MOD ODD  0000  1 ON  - $100000 PF2
BPLANE HEIGHT 0000  DISPLAY   DPF  2 ON  * $100000 PF1
PALETTE    $000000  PRIORITY  PF1  3 ON  * $100000 PF2
                    SAVE AS   IFF  4 OFF - $100000 PF1
                    status line..  5 OFF - $100000 PF2


SAVE AS:
 - IFF
 - RAW
 - RAW+  (with palette)
 - PAL   (just palette)

DISPLAY:
 - DPF   (dual playfiles)
 - HBR   (halfbrite)
 - HAM   (HAM)
 - STD   (standard..)

PRIORITY:
 - PF1
 - PF2

BPLANES
 - LORES
 - HIRES


#endif

#endif  /* _SEARCHER_H_INCLUDED */
