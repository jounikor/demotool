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

#include <graphics/gfxbase.h>
#include <graphics/gfxmacros.h>
/* Fix C= issues.. */
#if defined(GetOutlinePen)
#undef GetOutlinePen
#endif

#include <clib/alib_stdio_protos.h>

#include <graphics/copper.h>
#include <graphics/text.h>
#include <graphics/sprite.h>
#include <hardware/custom.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>

#include <proto/dos.h>
#include <dos/stdio.h>
#include <dos/dos.h>

#include "searcher.h"


/* Use the following to compile:
 *  vc +aos68km -lamiga -sd -sc -O2 -c99 screen.c  -o exe
 *
 *
 */

extern struct Custom custom;

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct DosLibrary *DOSBase;

struct TextFont *sp_font;

static struct TextAttr s_topaz = {
    "topaz.font",
    8,
    FS_NORMAL,
    FPF_ROMFONT
};

/*
 */

static UWORD s_head_palette[] = {0x0203, 0x0102, 0x0666, 0x0f66};

/* text area 1 */
static STRPTR s_text_area1[] = {"SCREEN WIDTH  0000",
                                "PFIELD1 WIDTH 0000",
                                "PFIELD2 WIDTH 0000",
                                "BPLANE HEIGHT 0000",
                                "                  ",
                                "                  ",
                                "ADDRESS    $000000"};

/* text area 2 */
static STRPTR s_text_area2[] = {"             ",
                                "RGB INDEX    ",
                                "MOD EVEN     ",
                                "MOD ODD      ",
                                "DISPLAY      ",
                                "PRIORITY     ",
                                "SAVE AS      "};

/* text area 3                   0 2   6 8 A*/
static STRPTR s_text_area3[] = {"BPLANES            ",
                                "0       $       PF1",
                                "1       $       PF2",
                                "2       $       PF1",
                                "3       $       PF2",
                                "4       $       PF1",
                                "5       $       PF2"};

static STRPTR s_text_reso[] = {"LORES","HIRES"};
static STRPTR s_text_ooff[] = {"ON ","OFF"};
static STRPTR s_text_lock[] = {"-","*"};

static STRPTR s_text_prio[] = {" PF1"," PF2"};
static STRPTR s_text_disp[] = {" STD"," HBR"," HAM","DUAL"};
static STRPTR s_text_save[] = {" RAW"," IFF","IFF+"," PAL"};

#define TEXT_VER_LEN 4
static STRPTR s_text_ver = "V3.0";
static STRPTR s_text_scx = "ScoopeX";


/* kind of a 'struct spriteimage' */

ULONG __chip sprites[8][57] = { 
    //ULONG __chip sprite0[] = {
    {0x00000000,  /* control words */
    0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,
    0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,

    0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,
    0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,

    0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,
    0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,

    0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,
    0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,
    0x00000000   /* end */
    },

    //ULONG __chip sprite1[] = {
    {0x00000000,  /* control words */
    0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,
    0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,

    0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,
    0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,

    0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,
    0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,

    0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,
    0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,
    0x00000000   /* end */
    },

    //ULONG __chip sprite2[] = {
    {0x00000000,  /* control words */
    0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,
    0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,

    0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,
    0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,

    0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,
    0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,

    0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,
    0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,
    0x00000000   /* end */
    },

    //ULONG __chip sprite3[] = {
    {0x00000000,  /* control words */
    0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,
    0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,

    0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,
    0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,

    0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,
    0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,

    0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,
    0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,
    0x00000000   /* end */
    },

    //ULONG __chip sprite4[] = {
    {0x00000000,  /* control words */
    0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,
    0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,

    0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,
    0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,

    0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,
    0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,

    0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,
    0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,
    0x00000000   /* end */
    },

    //ULONG __chip sprite5[] = {
    {0x00000000,  /* control words */
    0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,
    0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,

    0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,
    0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,

    0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,
    0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,

    0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,
    0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,
    0x00000000   /* end */
    },

    //ULONG __chip sprite6[] = {
    {0x00000000,  /* control words */
    0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,
    0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,

    0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,
    0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,

    0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,
    0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,

    0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,
    0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,0xfffc0000,
    0x00000000   /* end */
    },

    //ULONG __chip sprite7[] = {
    {0x00000000,  /* control words */
    0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,
    0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,

    0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,
    0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,

    0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,
    0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,

    0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,
    0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,0x0000fffc,
    0x00000000   /* end */
    }
};

static const struct ExtNewScreen s_newscreen = {
    0,0,                                        /* LeftEdge, TopEdge */
    640,64,2,                                   /* Width, Height, Depth */
    0,1,                                        /* DetailPen, BlockPen */
    //SCREENHIRES|SPRITES,                      /* ViewModes */
    0xc000,                                     /* ViewModes */
    CUSTOMSCREEN,                               /* Type */
    &s_topaz,                                   /* *Font */
    NULL,
    NULL,                                       /* *Gadgets */
    NULL,                                       /* *CustomBitmap */
    NULL                                        /* *Extension */
};



static const struct ExtNewWindow s_newwindow = {
    0,0,                                        /* LeftEdge, TopEdge */
    640,64,                                     /* Width, Height */
    JAM1,JAM2,                                  /* DetailPen, BlockPen */
    IDCMP_RAWKEY,                               /* IDCMPFlags */
    WFLG_BORDERLESS|WFLG_RMBTRAP|
    WFLG_ACTIVATE|WFLG_NOCAREREFRESH,           /* Flags */
    NULL,                                       /* *FirstGadget */
    NULL,                                       /* *CheckMark */
    NULL,                                       /* *Title */
    NULL,                                       /* *Screen */
    NULL,                                       /* *Bitmap */
    640,64,                                     /* MinWidth,MinHeight */
    640,64,                                     /* MaxWidth,MaxHeight */
    CUSTOMSCREEN,
    NULL                                        /* *Extension */
};

static struct Screen *sp_screen = NULL;
static struct Window *sp_window = NULL;
static struct SimpleSprite s_spr[8];





BOOL open_screen_window(void)
{
    sp_screen = OpenScreen((const struct NewScreen *)&s_newscreen);

    if (sp_screen == NULL) {
        return FALSE;
    }

    s_newwindow.Screen = sp_screen;
    sp_window = OpenWindow((struct NewWindow*)&s_newwindow);

    if (sp_window == NULL) {
        return FALSE;
    }

    LoadRGB4(&sp_screen->ViewPort,s_head_palette,4);

    return TRUE;
}


BOOL setup_sprites(void)
{
    SHORT spr_num;
    int n;

    for (n = 1; n < 8; n++) {
        spr_num = GetSprite(&s_spr[n],n);

        if (spr_num < 0) {
            Printf("Sprite%ld get failed\n",n);
        } else {
            s_spr[n].height = 11*5+1;
            s_spr[n].x = 28*(n-1);
            s_spr[n].y = 4;
            s_spr[n].num = n;
            ChangeSprite(&sp_screen->ViewPort,&s_spr[n],(void*)&sprites[n]);
        }
    }

    return TRUE;
}


void close_screen_window(void)
{
    int n;

    for (n = 1; n < 8; n++) {
        FreeSprite(n);
    }

    if (sp_window) {
        CloseWindow(sp_window);
    }
    if (sp_screen) {
        CloseScreen(sp_screen);
    }
}


static UWORD s_def_pal[] = {
    0x0000,0x0e60,0x0d50,0x0c40,0x0b30,0x0a20,0x0910,0x0800,
	0x00f7,0x00e6,0x00d5,0x00c4,0x00b3,0x00a2,0x0091,0x0080,
	0x0f7f,0x0e6e,0x0d5d,0x0c4c,0x0b3b,0x0a2a,0x0919,0x0808,
	0x007f,0x006e,0x005d,0x004c,0x003b,0x002a,0x0019,0x0008,
    0x0000,0x0000,0x0000    // last 3 are dummy bcos of sprites
};


static struct UCopList* setup_copper(struct Screen* p_scr, cop_cfg_t* p_cfg)
{
    struct UCopList* p_ucl = AllocMem(sizeof(*p_ucl), MEMF_PUBLIC|MEMF_CLEAR);
    struct ViewPort* p_vp;
    UWORD *p_pal = &p_cfg->pal[0];
    WORD shft;
    int y;
    WORD dffstrt, dffstop, step, word_cnt;
    WORD bplcon0;
    WORD bplcon2;

    if (p_ucl == NULL) {
        return NULL;
    }

    p_vp = &p_scr->ViewPort;

    /* DFF100 vale */
    bplcon0 = 0x0200;
    bplcon2 = 0x0000;

    if (p_cfg->flags & MODE_HAM) {
        bplcon0 |= 0x0800;
    }
    if (p_cfg->flags & MODE_HIRES) {
        bplcon0 |= 0x8000;
    }
    if (p_cfg->flags & MODE_DUALPF) {
        bplcon0 |= 0x4000;
    }
    if (p_cfg->flags & MODE_PF2PRI) {
        bplcon2 |= 0x0040;
    }

    bplcon0 |= p_cfg->num_bpl << 12;

    /* DFF102 value */
    shft = p_cfg->pf2shft<<3 | p_cfg->pf1shft;
    
    /* DFF08e and DFF090 values.. both lower and hires */
    word_cnt = p_cfg->scr_width >> 4;
    //dffstrt = p_cfg->dffstrt;
    
    if (p_cfg->flags & MODE_HIRES) {
        if (word_cnt > 40) {
            word_cnt = 40;
        }
        if (word_cnt < 3) {
            word_cnt = 3;
        }

        dffstrt = 0x3c;
        word_cnt -= 2;
        step = 4;
    } else {
        if (word_cnt > 20) {
            word_cnt = 20;
        }
        if (word_cnt < 2) {
            word_cnt = 2;
        }
        
        dffstrt = 0x38;
        word_cnt -= 1;
        step = 8;
    }

    dffstop = dffstrt + step*word_cnt;

    CINIT(p_ucl,2048);

    /* A discovery.. system copperlist entries are place before the
     * first user defined CWAIT()
     * As we know we asked for a 2 color hires we force the ECS features off
     * by reissuing write to BPLCON0.. hacky hacky..
     */
    CWAIT(p_ucl,0,6);
    CMove(p_ucl,(void*)0xdff100,0xa200); CBump(p_ucl);
    CMove(p_ucl,(void*)0xdff180,0x0fff); CBump(p_ucl);
    CWAIT(p_ucl,1,6);
    CMove(p_ucl,(void*)0xdff180,0x0203); CBump(p_ucl);
    /* for sprite priority behind graphics or  */
    CMove(p_ucl,(void*)0xdff104,0x0000); CBump(p_ucl);

    for (y = 0; y < 5; y++) {
        CWAIT(p_ucl,4+y*11,0);
        CMove(p_ucl,(void*)0xdff1a4,*p_pal++); CBump(p_ucl);   // spr1 col2
        CMove(p_ucl,(void*)0xdff1aa,*p_pal++); CBump(p_ucl);   // spr2 col1
        CMove(p_ucl,(void*)0xdff1ac,*p_pal++); CBump(p_ucl);   // spr3 col2
        CMove(p_ucl,(void*)0xdff1b2,*p_pal++); CBump(p_ucl);   // spr4 col1
        CMove(p_ucl,(void*)0xdff1b4,*p_pal++); CBump(p_ucl);   // spr5 col2
        CMove(p_ucl,(void*)0xdff1ba,*p_pal++); CBump(p_ucl);   // spr6 col1
        CMove(p_ucl,(void*)0xdff1bc,*p_pal++); CBump(p_ucl);   // spr7 col2
    }

    CWAIT(p_ucl,62,0);
    CMove(p_ucl,(void*)0xdff100,0x0200); CBump(p_ucl);
    CMove(p_ucl,(void*)0xdff102,shft); CBump(p_ucl);
    CMove(p_ucl,(void*)0xdff180,0x0fff); CBump(p_ucl);
    CMove(p_ucl,(void*)0xdff108,p_cfg->mod_odd); CBump(p_ucl);
    CMove(p_ucl,(void*)0xdff10a,p_cfg->mod_even); CBump(p_ucl);
    CMove(p_ucl,(void*)0xdff08e,0x2c81); CBump(p_ucl);
    CMove(p_ucl,(void*)0xdff090,0x38c1); CBump(p_ucl);
    CMove(p_ucl,(void*)0xdff092,dffstrt); CBump(p_ucl);
	CMove(p_ucl,(void*)0xddf094,dffstop); CBump(p_ucl);
    CMove(p_ucl,(void*)0xdff104,bplcon2); CBump(p_ucl);

    for (y = 0; y < 6; y++) {
        ULONG ptr = 0xdff0e0 + 4*y;
        LONG  bpl = p_cfg->bpl_ptr[y] & 0x1ffffe;
    	CMove(p_ucl,(void*)(ptr+0),bpl>>16); CBump(p_ucl);
    	CMove(p_ucl,(void*)(ptr+2),bpl);    CBump(p_ucl);
    }
    for (y = 1; y < 32; y++) {
        ULONG ptr = 0xdff180 + 2*y;
    	CMove(p_ucl,(void*)ptr,p_cfg->pal[y]); CBump(p_ucl);
    }

    CWAIT(p_ucl,63,6);
    CMove(p_ucl,(void*)0xdff100,bplcon0); CBump(p_ucl);
  	CMove(p_ucl,(void*)0xdff180,p_cfg->pal[0]); CBump(p_ucl);

    /*
     * Another dirty hack.. we let the user copperlist leak over the
     * opened screen size. This works if we keep using 1.x compatible
     * screens..
     */
    CWAIT(p_ucl,264,6);
    CMove(p_ucl,(void*)0xdff180,0x0fff); CBump(p_ucl);
    CMove(p_ucl,(void*)0xdff100,0x0200); CBump(p_ucl);
    CWAIT(p_ucl,265,6);
    CMove(p_ucl,(void*)0xdff180,0x0203); CBump(p_ucl);

    CEND(p_ucl);

    /* */

    Forbid();

    if (p_vp->UCopIns) {
        FreeVPortCopLists(p_vp);
        MakeScreen(p_scr);
    }

    p_vp->UCopIns = p_ucl;
    RethinkDisplay();
    Permit();

#if 0
    /* dump */
    ULONG *p_d = ((ULONG*)0x80000);
    ULONG *p_s = (ULONG*)GfxBase->LOFlist;

    do {
        *p_d++ = *p_s;
    } while (*p_s++ != 0xfffffffe);

#endif

    return p_ucl;
}


static void update_head_texts(BOOL statics, ULONG texts, cop_cfg_t *p_cfg) 
{
    int n,x,y;
    char buf[10];

    if (statics) {
        SetDrMd(sp_window->RPort,JAM2);
        SetAPen(sp_window->RPort,2);
        SetBPen(sp_window->RPort,1);

        /* Area 1 */
        x = TEXT_AREA1_X;
        y = TEXT_AREA1_Y;

        for (n = 0; n < 7; n++) {
            Move(sp_window->RPort,x,y); y += 8;
            Text(sp_window->RPort,s_text_area1[n],TEXT_AREA1_LEN);
        }

        /* Area 2 */
        x = TEXT_AREA2_X;
        y = TEXT_AREA2_Y;

        for (n = 0; n < 7; n++) {
            Move(sp_window->RPort,x,y); y += 8;
            Text(sp_window->RPort,s_text_area2[n],TEXT_AREA2_LEN);
        }

        /* Area 3 */
        x = TEXT_AREA3_X;
        y = TEXT_AREA3_Y;

        for (n = 0; n < 7; n++) {
            Move(sp_window->RPort,x,y); y += 8;
            Text(sp_window->RPort,s_text_area3[n],TEXT_AREA3_LEN);
        }

        /* scoopex */
        SetAPen(sp_window->RPort,3);
        SetDrMd(sp_window->RPort,JAM1);

        y = TEXT_AREA3_Y;

        for (n = 0; n < 7; n++) {
            Move(sp_window->RPort,640-8,y); y += 8;
            Text(sp_window->RPort,&s_text_scx[n],1);
        }

        /* version */
        Move(sp_window->RPort,140,56);
        Text(sp_window->RPort,s_text_ver,TEXT_VER_LEN);
    }

    SetDrMd(sp_window->RPort,JAM2);
    SetAPen(sp_window->RPort,2);
    SetBPen(sp_window->RPort,1);

    /* Area 1 */
    if (texts & TEXT_SCREEN) {
        sprintf(buf,"%04d",p_cfg->scr_width);
        Move(sp_window->RPort,TEXT_AREA1_X+14*8,TEXT_AREA1_Y+0*8);
        Text(sp_window->RPort,buf,4);
    }
    if (texts & TEXT_PF1WIDTH) {
        sprintf(buf,"%04d",p_cfg->scr_width+p_cfg->mod_odd);
        Move(sp_window->RPort,TEXT_AREA1_X+14*8,TEXT_AREA1_Y+1*8);
        Text(sp_window->RPort,buf,4);
    }
    if (texts & TEXT_PF2WIDTH) {
        sprintf(buf,"%04d",p_cfg->scr_width+p_cfg->mod_even);
        Move(sp_window->RPort,TEXT_AREA1_X+14*8,TEXT_AREA1_Y+2*8);
        Text(sp_window->RPort,buf,4);
    }
    if (texts & TEXT_HEIGHT) {
        sprintf(buf,"%04d",p_cfg->scr_height);
        Move(sp_window->RPort,TEXT_AREA1_X+14*8,TEXT_AREA1_Y+3*8);
        Text(sp_window->RPort,buf,4);
    }

    /* Area 2 */
    if (texts & TEXT_RGBIDX) {
        sprintf(buf,"%02d",p_cfg->rgb_idx);
        Move(sp_window->RPort,TEXT_AREA2_X+11*8,TEXT_AREA2_Y+1*8);
        Text(sp_window->RPort,buf,2);
    }
    if (texts & TEXT_MODEVEN) {
        sprintf(buf,"%04d",p_cfg->mod_even);
        Move(sp_window->RPort,TEXT_AREA2_X+9*8,TEXT_AREA2_Y+2*8);
        Text(sp_window->RPort,buf,4);
    }
    if (texts & TEXT_MODODD) {
        sprintf(buf,"%04d",p_cfg->mod_odd);
        Move(sp_window->RPort,TEXT_AREA2_X+9*8,TEXT_AREA2_Y+3*8);
        Text(sp_window->RPort,buf,4);
    }
    if (texts & TEXT_DISPLAY) {
        Move(sp_window->RPort,TEXT_AREA2_X+9*8,TEXT_AREA2_Y+4*8);
        Text(sp_window->RPort,s_text_disp[p_cfg->display],4);
    }
    if (texts & TEXT_PRIORITY) {
        Move(sp_window->RPort,TEXT_AREA2_X+9*8,TEXT_AREA2_Y+5*8);
        Text(sp_window->RPort,s_text_prio[p_cfg->priority],4);
    }
    if (texts & TEXT_SAVEAS) {
        Move(sp_window->RPort,TEXT_AREA2_X+9*8,TEXT_AREA2_Y+6*8);
        Text(sp_window->RPort,s_text_save[p_cfg->save_as],4);
    }

    /* Area 3 */
    if (texts & TEXT_BPLANES) {
        /* num planes */
        Move(sp_window->RPort,TEXT_AREA3_X+8*8,TEXT_AREA3_Y+0*8);
        sprintf(buf,"%d",p_cfg->num_bpl);
        Text(sp_window->RPort,buf,1);

        /* on/off info */
        y = TEXT_AREA3_Y+1*8;
        for (n = 0; n < 6; n++) {
            Move(sp_window->RPort,TEXT_AREA3_X+2*8,y);
            x =  n < p_cfg->num_bpl ? 0 : 1;
            y += 8;
            Text(sp_window->RPort,s_text_ooff[x],3);
        }
    }
    if (texts & TEXT_RESO) {
        /* lores or hires */
        n = p_cfg->flags & MODE_HIRES ? 1 : 0;
        Move(sp_window->RPort,TEXT_AREA3_X+10*8,TEXT_AREA3_Y+0*8);
        Text(sp_window->RPort,s_text_reso[n],5);
    }
    if (texts & TEXT_BPLLOCKS) {
        /* bpl locks */
        y = TEXT_AREA3_Y+1*8;
        x = p_cfg->bpl_lock;
        for (n = 0; n < 6; n++) {
            Move(sp_window->RPort,TEXT_AREA3_X+6*8,y);
            Text(sp_window->RPort,s_text_lock[x&1],1);
            y += 8; x >>= 1;
        }
    }
    if (texts & TEXT_BPLADDRS) {
        /* bpl locks and addresses */
        y = TEXT_AREA3_Y + 1*8;

        for (n = 0; n < 6; n++) {
            Move(sp_window->RPort,TEXT_AREA3_X+9*8,y);
            sprintf(buf,"%06lx",p_cfg->bpl_ptr[n] & 0x1ffffe);
            Text(sp_window->RPort,buf,6);
            y += 8;
        }
    }

    return;
}

static void update_pal_texts(BOOL on)
{
    char buf[] = "   ";
    int x,y,n;
    n = 0; x = 0; y = 0;

    SetDrMd(sp_window->RPort,JAM2);
    SetAPen(sp_window->RPort,3);
    SetBPen(sp_window->RPort,0);

    while (n < 32) {
        if (on) {
          sprintf(buf,"%02d",n);
        }        

        if (x == 7) {
            x = 0; ++y;
        }

        Move(sp_window->RPort,TEXT_AREA0_X+x*28,TEXT_AREA0_Y+y*11);
        Text(sp_window->RPort,buf,2);
        ++n; ++x;
    }
}



static void init_cop_cfg(cop_cfg_t *p_cfg, BOOL pal_only)
{
    int n;

    for (n = 0; n < 32+3; n++) {
        p_cfg->pal[n] = s_def_pal[n];
    }

    if (pal_only) {
        return;
    }

    for (n = 0; n < 6; n++) {
        p_cfg->bpl_ptr[n] = 0x4000;
    }

    p_cfg->mod_even = 0;
    p_cfg->mod_odd = 0;

    p_cfg->scr_width = 320;
    p_cfg->scr_height = 200;

    p_cfg->display  = IDX_DISP_STD;
    p_cfg->priority = IDX_PRIO_PF1;
    p_cfg->save_as  = IDX_SAVE_RAW;

    p_cfg->pf1shft = 0;
    p_cfg->pf2shft = 0;
    p_cfg->num_bpl = 5;
    p_cfg->flags = 0;
    p_cfg->bpl_lock = 0x3f; /* all six planes */

    p_cfg->key_state = STM_MAIN;
    p_cfg->rgb_idx = 0;
}

static ULONG inc_scr_width(cop_cfg_t *p_cfg)
{
    /* Also take overscan into account */
    WORD new_width = p_cfg->scr_width;
    WORD max;

    if (p_cfg->flags & MODE_HIRES) {
        new_width += 32;
        max = 640+96;
    } else {
        new_width += 32;
        max = 320+48;
    }

    p_cfg->scr_width = new_width < max ? new_width : max;
    return TEXT_SCREEN|TEXT_PF1WIDTH|TEXT_PF2WIDTH;
}

static ULONG dec_scr_width(cop_cfg_t *p_cfg)
{
    WORD new_width = p_cfg->scr_width;
    WORD min;

    if (p_cfg->flags & MODE_HIRES) {
        new_width -= 32;
        min = 64;
    } else {
        new_width -= 16;
        min = 32;
    }

    p_cfg->scr_width = new_width > min ? new_width : min;
    return TEXT_SCREEN|TEXT_PF1WIDTH|TEXT_PF2WIDTH;
}

static ULONG reset_bplane_addrs(cop_cfg_t *p_cfg)
{
    p_cfg->bpl_ptr[1] = p_cfg->bpl_ptr[0];
    p_cfg->bpl_ptr[2] = p_cfg->bpl_ptr[0];
    p_cfg->bpl_ptr[3] = p_cfg->bpl_ptr[0];
    p_cfg->bpl_ptr[4] = p_cfg->bpl_ptr[0];
    p_cfg->bpl_ptr[5] = p_cfg->bpl_ptr[0];
    return TEXT_BPLADDRS;
}


static ULONG set_bplane_lock(cop_cfg_t *p_cfg, int bpl_idx)
{
    if (bpl_idx > 5) {
        if (p_cfg->bpl_lock == 0x3f) {
            p_cfg->bpl_lock = 0;
        } else {
            p_cfg->bpl_lock = 0x3f;
        }
    } else {
        p_cfg->bpl_lock ^= (1 << bpl_idx);
    }
    return TEXT_BPLLOCKS;
}

static ULONG handle_display_mode(cop_cfg_t *p_cfg)
{
    if (++p_cfg->display > IDX_DISP_MAX) {
        p_cfg->display = 0;
    }
    return TEXT_DISPLAY;
}


static ULONG set_bplane_height(cop_cfg_t *p_cfg, WORD height)
{
    p_cfg->scr_height = height;
    return TEXT_HEIGHT;
}

static ULONG dec_bplane_num(cop_cfg_t *p_cfg)
{
    if (p_cfg->num_bpl > 0) {
        --p_cfg->num_bpl;
        return TEXT_BPLANES;
    }
    return 0;
}

static ULONG inc_bplane_num(cop_cfg_t *p_cfg)
{
    /* This allows OCS/ECS 7bpl mode.. */
    if (p_cfg->num_bpl < 7) {
        ++p_cfg->num_bpl;
        return TEXT_BPLANES;
    }
    return 0;
}

static ULONG handle_bplane_move(cop_cfg_t *p_cfg, int key, int qual)
{
    LONG step_odd=0, step_even=0;
    int n,m=1;

    switch (key) {
    case KEY_LEFT:
        step_even  = 16;
        step_odd   = 16;
        break;
    case KEY_RIGHT:
        step_even  = -16;
        step_odd   = -16;
        break;
    case KEY_DOWN:
        step_even = -(p_cfg->scr_width + p_cfg->mod_even);
        step_odd  = -(p_cfg->scr_width + p_cfg->mod_odd);
        break;
    case KEY_UP:
        step_even = p_cfg->scr_width + p_cfg->mod_even;
        step_odd  = p_cfg->scr_width + p_cfg->mod_odd;
        break;
    default:
        return 0;
    }

    step_even >>= 3;
    step_odd  >>= 3;

    if (!(p_cfg->flags & MODE_DUALPF)) {
        step_even = step_odd;
    }
    if (qual & IEQUALIFIER_RSHIFT) {
        step_even = step_even * (p_cfg->scr_height >> 3);
        step_odd  = step_odd  * (p_cfg->scr_height >> 3);
        m = 1;
    } else if (qual & IEQUALIFIER_LSHIFT) {
        step_even = step_even * (p_cfg->scr_height >> 2);
        step_odd  = step_odd  * (p_cfg->scr_height >> 2);
        m = 1;
    }
    if (qual & IEQUALIFIER_REPEAT) {
        m = 4;
    }

    for (n = 0; n < 6; n++) {
        if (p_cfg->bpl_lock & (1 << n)) {
            if (n & 1) {
                /* even bitplane */
                p_cfg->bpl_ptr[n] += (step_even * m);
            } else {
                /* odd bitplane */
                p_cfg->bpl_ptr[n] += (step_odd * m);
            }
        }
    }

    return TEXT_BPLADDRS;
}

static ULONG set_resolution(cop_cfg_t *p_cfg, BOOL hires)
{
    ULONG ret = TEXT_RESO|TEXT_SCREEN;

    if (hires) {
        p_cfg->flags |= MODE_HIRES;
        p_cfg->scr_width = 640;
        
        if (p_cfg->num_bpl > 4) {
            p_cfg->num_bpl = 4;
            ret |= TEXT_BPLANES;
        }
    } else {
        p_cfg->flags &= ~MODE_HIRES;
        p_cfg->scr_width = 320;
    }
    return ret;
}




static BOOL main_key_loop(int key, int qual, cop_cfg_t *p_cfg)
{
    static BOOL onoff = TRUE;
    ULONG update = 0;
    ULONG move = 0;

    switch (key) {
    case KEY_F10:
        return TRUE;
    case KEY_NP_FS:
        update_pal_texts(onoff);
        onoff = !onoff;
        break;
    case KEY_1: case KEY_2: case KEY_3: case KEY_4: 
    case KEY_5: case KEY_6: case KEY_7: 
        update = set_bplane_lock(p_cfg,key-KEY_1);
        break;
    case KEY_8:
        update = reset_bplane_addrs(p_cfg);
        break;
    case KEY_F1:
        update = handle_display_mode(p_cfg);
        break;
    case KEY_F2:
        update = set_bplane_height(p_cfg,100);
        break;
    case KEY_F3:
        update = set_bplane_height(p_cfg,200);
        break;
    case KEY_F4:
        update = set_bplane_height(p_cfg,256);
        break;
    case KEY_F5:
        update = set_bplane_height(p_cfg,512);
        break;
    case KEY_MINUS:
        update = dec_bplane_num(p_cfg);
        break;
    case KEY_EQUAL:
        update = inc_bplane_num(p_cfg);
        break;
    case KEY_LBRA:
        update = dec_scr_width(p_cfg);
        break;
    case KEY_RBRA:
        update = inc_scr_width(p_cfg);
        break;
    case KEY_H:
        update = set_resolution(p_cfg,TRUE);
        break;
    case KEY_L:
        update = set_resolution(p_cfg,FALSE);
        break;

    case KEY_SLASH:
        break;
    case KEY_FSTOP:
        break;
    case KEY_COMMA:
        break;


    case KEY_UP:
    case KEY_DOWN:
    case KEY_LEFT:
    case KEY_RIGHT:
        update = handle_bplane_move(p_cfg,key,qual);
        break;

    case KEY_F6:
        p_cfg->key_state = STM_HEIGHT;
        break;

    case KEY_ESC:
    case KEY_RETURN:
        p_cfg->key_state = STM_MAIN;
        break;

    default:
        break;
    }

    if (update) {
        update_head_texts(FALSE,update,p_cfg);
    }
    if (update & TEXT_COPPER_MASK) {
        setup_copper(sp_screen,p_cfg);
    }

    return FALSE;
}




int main(char *argv)
{
    struct IntuiMessage *p_msg;
    ULONG win_sig_bit, wait_mask, sig;
    BOOL done = FALSE;
    struct UCopList* p_ucl;
    cop_cfg_t cfg;
    int key, qual;

    GfxBase = (struct GfxBase*)OpenLibrary("graphics.library",0);
    IntuitionBase = (struct IntuitionBase*)OpenLibrary("intuition.library",0);
    DOSBase = (struct DosLibrary*)OpenLibrary("dos.library",0);

    sp_font = OpenFont(&s_topaz);
    init_cop_cfg(&cfg,FALSE);


    if (GfxBase && IntuitionBase && DOSBase && sp_font) {
        open_screen_window();
        update_head_texts(TRUE,~0,&cfg); 
        setup_sprites();
        p_ucl = setup_copper(sp_screen, &cfg);

        win_sig_bit = sp_window->UserPort->mp_SigBit;
        wait_mask = 1L << win_sig_bit;

        while (!done) {
            sig = Wait(wait_mask);

            if (sig & (1L << win_sig_bit)) {
                while (p_msg = (struct IntuiMessage *)GetMsg(sp_window->UserPort)) {
                    key = -1;

                    if (p_msg->Class == IDCMP_RAWKEY) {
                        key = p_msg->Code;
                        qual = p_msg->Qualifier;
                    }

                    ReplyMsg((struct Message *)p_msg);
                    
                    if (key < 0) {
                        continue;
                    }
                    
                    switch (cfg.key_state) {
                    case STM_MAIN:
                        done = main_key_loop(key,qual,&cfg);
                        break;
                    case STM_NUMPAD:
                        break;
                    case STM_SEARCH:
                        break;
                    case STM_HEIGHT:
                        break;
                    default:
                        break;
                    }
                }
            }
        }
    } else {
        WriteStr("Pieleen meni\n");
    }

    if (p_ucl) {
        FreeVPortCopLists(&sp_screen->ViewPort);
        RemakeDisplay();
    }


    close_screen_window();

    if (sp_font) {
        CloseFont(sp_font);
    }
    if (DOSBase) {
        CloseLibrary((struct Library*)DOSBase);
    }
    if (GfxBase) {
        CloseLibrary((struct Library*)GfxBase);
    }
    if (IntuitionBase) {
        CloseLibrary((struct Library*)IntuitionBase);
    }


    return RETURN_OK;
}

