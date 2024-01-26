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

#include <proto/exec.h>
#define __NOLIBBASE__
#include <proto/graphics.h>
#include <proto/intuition.h>

#include <exec/execbase.h>
#include <graphics/copper.h>
#include <graphics/text.h>
#include <graphics/sprite.h>
#include <graphics/view.h>
#include <hardware/custom.h>

#include <clib/alib_stdio_protos.h>
#include <clib/macros.h>
#include <string.h>

#include "searcher.h"
#include "screen.h"

/* Use the following to compile:
 */

extern struct Custom custom;
extern struct GfxBase *GfxBase;
extern struct IntuitionBase *IntuitionBase;

/* text area 1 */
static STRPTR s_text_area1[] = {"SCREEN WIDTH      ",
                                "PFIELD1 WIDTH     ",
                                "PFIELD2 WIDTH     ",
                                "BPLANE HEIGHT     ",
                                "PALETTE    $      "};

/* text area 2 */
static STRPTR s_text_area2[] = {"RGB  #   $   ",
                                "MOD EVEN     ",
                                "MOD ODD      ",
                                "DISPLAY      ",
                                "PRIORITY     ",
                                "SAVE AS      ",
                                "             "};

/* text area 3                   0 2   6 8   C E    */
static STRPTR s_text_area3[] = {"BITPLANES  #       ",
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
static STRPTR s_text_disp[] = {" STD"," HAM","DUAL"};
static STRPTR s_text_save[] = {" IFF"," RAW"," PAL"};

/*                                 0123456789abc */
static STRPTR s_status_lines[] = {"Searcher v"MSTR(MAJOR)"."MSTR(MINOR),
                                  "Palette found",
                                  "No file saved",
                                  "File saved OK",
                                  "Everything OK",
                                  "Out of memory",
                                  "Internal fail",
                                  "File exists..",
                                  "Nothing found"};

static STRPTR s_text_scx = "ScoopeX";

static UWORD s_def_pal[] = {
    0x0000,0x0e60,0x0d50,0x0c40,0x0b30,0x0a20,0x0910,0x0800,
	0x00f7,0x00e6,0x00d5,0x00c4,0x00b3,0x00a2,0x0091,0x0080,
	0x0f7f,0x0e6e,0x0d5d,0x0c4c,0x0b3b,0x0a2a,0x0919,0x0808,
	0x007f,0x006e,0x005d,0x004c,0x003b,0x002a,0x0019,0x0008,
    0x0203,0x0203,0x0203    // last 3 are dummy bcos of sprites
};


/* kind of a 'struct spriteimage' */

ULONG __chip sprites[7][57] = { 
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


static struct SimpleSprite s_spr[7];


/* Spaghetti code starts.. there are hardly comments.. */

BOOL setup_sprites(struct Screen *p_screen)
{
    SHORT spr_num;
    int n;

    /* sprites 1 to 7.. in C, though, indexed 0 to 6. */

    for (n = 0; n < 7; n++) {
        spr_num = GetSprite(&s_spr[n],n+1);

        if (spr_num >= 0) {
            s_spr[n].height = 11*5+0;   // was 1
            s_spr[n].x = 28*n;
            s_spr[n].y = 4;
            s_spr[n].num = n+1;
            ChangeSprite(&p_screen->ViewPort,&s_spr[n],(void*)&sprites[n]);
        }
    }

    return TRUE;
}


struct UCopList* setup_copper(struct Screen* p_scr, cop_cfg_t* p_cfg)
{
    struct UCopList* p_ucl = AllocMem(sizeof(*p_ucl), MEMF_PUBLIC|MEMF_CLEAR);
    struct ViewPort* p_vp;
    UWORD *p_pal = &p_cfg->pal[0];
    WORD shft, height;
    int y;
    WORD dffstrt, dffstop, step, word_cnt;
    WORD bplcon0;
    WORD bplcon2;

    if (p_ucl == NULL) {
        return NULL;
    }

    p_vp = &p_scr->ViewPort;

    /* DFF100 value..  */
    bplcon0 = 0x0200;
    bplcon2 = 0x0000;

    if (p_cfg->flags & MODE_HAM) {
        bplcon0 |= 0x0800;
    }
    if (p_cfg->flags & MODE_HIRES) {
        bplcon0 |= 0x8000;
    }
    if (p_cfg->flags & MODE_DUALPF) {
        bplcon0 |= 0x0400;
    }
    if (p_cfg->flags & MODE_PF2PRI) {
        bplcon2 |= 0x0040;
    }

    bplcon0 |= p_cfg->num_bpl << 12;

    /* DFF102 value */
    shft = p_cfg->pf2shft<<4 | p_cfg->pf1shft;
    
    /* DFF08e and DFF090 values.. both lower and hires */
    word_cnt = p_cfg->scr_width >> 4;
    
    if (p_cfg->flags & MODE_HIRES) {
        if (word_cnt > 40) {
            word_cnt = 40;
        }
        if (word_cnt < 3) {
            word_cnt = 3;
        }

        dffstrt = 0x3c-4;
        word_cnt -= 2;
        step = 4;
    } else {
        if (word_cnt > 20) {
            word_cnt = 20;
        }
        if (word_cnt < 2) {
            word_cnt = 2;
        }
        
        dffstrt = 0x38-8;
        word_cnt -= 1;
        step = 8;
    }

    dffstop = dffstrt + step*word_cnt;

    CINIT(p_ucl,160);

    /* A discovery.. system copperlist entries are place before the
     * first user defined CWAIT()
     * As we know we asked for a 2 color hires we force the ECS features off
     * by reissuing write to BPLCON0.. hacky hacky..
     */
    CWAIT(p_ucl,0,0);
    CMove(p_ucl,(void*)0xdff100,0xa200); CBump(p_ucl);
    CMove(p_ucl,(void*)0xdff180,0x0fff); CBump(p_ucl);
    CWAIT(p_ucl,1,6);
    CMove(p_ucl,(void*)0xdff180,0x0203); CBump(p_ucl);
    /* for sprite priority behind graphics or  */
    CMove(p_ucl,(void*)0xdff104,0x0000); CBump(p_ucl);  // 6

    for (y = 0; y < 5; y++) {
        CWAIT(p_ucl,4+y*11,0);
        CMove(p_ucl,(void*)0xdff1a4,*p_pal++); CBump(p_ucl);   // spr1 col2
        CMove(p_ucl,(void*)0xdff1aa,*p_pal++); CBump(p_ucl);   // spr2 col1
        CMove(p_ucl,(void*)0xdff1ac,*p_pal++); CBump(p_ucl);   // spr3 col2
        CMove(p_ucl,(void*)0xdff1b2,*p_pal++); CBump(p_ucl);   // spr4 col1
        CMove(p_ucl,(void*)0xdff1b4,*p_pal++); CBump(p_ucl);   // spr5 col2
        CMove(p_ucl,(void*)0xdff1ba,*p_pal++); CBump(p_ucl);   // spr6 col1
        CMove(p_ucl,(void*)0xdff1bc,*p_pal++); CBump(p_ucl);   // spr7 col2
    } // 40

    CWAIT(p_ucl,62,0);
    CMove(p_ucl,(void*)0xdff100,0x0200); CBump(p_ucl);
    CMove(p_ucl,(void*)0xdff102,shft); CBump(p_ucl);
    CMove(p_ucl,(void*)0xdff180,0x0fff); CBump(p_ucl);
    CMove(p_ucl,(void*)0xdff108,p_cfg->mod_odd); CBump(p_ucl);
    CMove(p_ucl,(void*)0xdff10a,p_cfg->mod_even); CBump(p_ucl);
    // This is offending copper move under kick1.3.. why?
    //CMove(p_ucl,(void*)0xdff08e,0x2c81); CBump(p_ucl);
    // Another attemps
    CMove(p_ucl,(void*)0xdff08e,0x0571); CBump(p_ucl);
    CMove(p_ucl,(void*)0xdff090,0x7fc1); CBump(p_ucl);
    CMove(p_ucl,(void*)0xdff092,dffstrt); CBump(p_ucl);
	CMove(p_ucl,(void*)0xddf094,dffstop); CBump(p_ucl);
    CMove(p_ucl,(void*)0xdff104,bplcon2); CBump(p_ucl); // 11

    for (y = 0; y < 6; y++) {
        ULONG ptr = 0xdff0e0 + 4*y;
        LONG  bpl = p_cfg->bpl_ptr[y] & 0x1ffffe;
    	CMove(p_ucl,(void*)(ptr+0),bpl>>16); CBump(p_ucl);
    	CMove(p_ucl,(void*)(ptr+2),bpl);    CBump(p_ucl);   // 12
    }
    for (y = 1; y < 32; y++) {
        ULONG ptr = 0xdff180 + 2*y;
    	CMove(p_ucl,(void*)ptr,p_cfg->pal[y]); CBump(p_ucl);
    }   // 32

    /* make sure the CWAIT on the last line is after the possiblr
     * OS inserted bitplane disabling.. this is needed for 1.3. In
     * 3.1 and better the OS seems to skip disabling if there are
     * CWAITs beyond the viewport..
     */
    CWAIT(p_ucl,63,10);
    CMove(p_ucl,(void*)0xdff100,bplcon0); CBump(p_ucl);
  	CMove(p_ucl,(void*)0xdff180,p_cfg->pal[0]); CBump(p_ucl);   // 3

    /*
     * Another dirty hack.. we let the user copperlist leak over the
     * opened screen size. This works if we keep using 1.x compatible
     * screens.. However, with kickstart 1.3 one need to make sure the
     * bitplanes are re-enabled as the OS insist on disable them even
     * if there is a CWAIT beyond the viewport..
     */
    height = p_cfg->scr_height + 63;

    if (height > 263) {
        height = 263;
    }

    CWAIT(p_ucl,height,0);
    CMove(p_ucl,(void*)0xdff100,0x0200); CBump(p_ucl);
    CMove(p_ucl,(void*)0xdff180,0x0fff); CBump(p_ucl);
    CWAIT(p_ucl,height+1,0);
    CMove(p_ucl,(void*)0xdff180,0x0203); CBump(p_ucl);

    CEND(p_ucl);    // 6


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
    {
        ULONG *s = (ULONG*)GfxBase->LOFlist;
        ULONG *d = (ULONG*)0x180000;

        *d++ = (ULONG)GfxBase;

        do {
            *d++ = *s;
        } while (*s++ != 0xfffffffe);
    }
#endif

    return p_ucl;
}


void update_head_texts(struct Window *p_window, BOOL statics, ULONG texts, cop_cfg_t *p_cfg) 
{
    int n,x,y,width;
    char buf[20];

    if (statics) {
        SetDrMd(p_window->RPort,JAM2);
        SetAPen(p_window->RPort,1);
        SetBPen(p_window->RPort,2);

        /* Area 1 */
        x = TEXT_AREA1_X;
        y = TEXT_AREA1_Y;

        for (n = 0; n < 5; n++) {
            Move(p_window->RPort,x,y); y += 8;
            Text(p_window->RPort,s_text_area1[n],TEXT_AREA1_LEN);
        }

        /* Area 2 */
        x = TEXT_AREA2_X;
        y = TEXT_AREA2_Y;

        for (n = 0; n < 7; n++) {
            Move(p_window->RPort,x,y); y += 8;
            Text(p_window->RPort,s_text_area2[n],TEXT_AREA2_LEN);
        }

        /* Area 3 */
        x = TEXT_AREA3_X;
        y = TEXT_AREA3_Y;

        for (n = 0; n < 7; n++) {
            Move(p_window->RPort,x,y); y += 8;
            Text(p_window->RPort,s_text_area3[n],TEXT_AREA3_LEN);
        }

        /* scoopex */
        SetAPen(p_window->RPort,3);
        SetDrMd(p_window->RPort,JAM1);

        y = TEXT_AREA3_Y;

        for (n = 0; n < 7; n++) {
            Move(p_window->RPort,640-8,y); y += 8;
            Text(p_window->RPort,&s_text_scx[n],1);
        }

        /* Filename */
        SetAPen(p_window->RPort,1);
        Move(p_window->RPort,132,56);
        Text(p_window->RPort,"FILENAME",8);
    }

    SetDrMd(p_window->RPort,JAM2);
    SetAPen(p_window->RPort,1);
    SetBPen(p_window->RPort,2);

    /* Area 1 */
    if (texts & TEXT_SCREEN) {
        sprintf(buf,"%4d",p_cfg->scr_width);
        Move(p_window->RPort,TEXT_AREA1_X+14*8,TEXT_AREA1_Y+0*8);
        Text(p_window->RPort,buf,4);
    }
    if (texts & TEXT_PF1WIDTH || texts & TEXT_MODODD) {
        width = p_cfg->scr_width+p_cfg->mod_odd;
        if (width < p_cfg->scr_width) {
            width = p_cfg->scr_width;
        }

        sprintf(buf,"%4d",width);
        Move(p_window->RPort,TEXT_AREA1_X+14*8,TEXT_AREA1_Y+1*8);
        Text(p_window->RPort,buf,4);
    }
    if (texts & TEXT_PF2WIDTH || texts & TEXT_MODEVEN) {
        width = p_cfg->scr_width+p_cfg->mod_even;
        if (width < p_cfg->scr_width) {
            width = p_cfg->scr_width;
        }

        sprintf(buf,"%4d",width);
        Move(p_window->RPort,TEXT_AREA1_X+14*8,TEXT_AREA1_Y+2*8);
        Text(p_window->RPort,buf,4);
    }
    if (texts & TEXT_HEIGHT) {
        sprintf(buf,"%4d",p_cfg->scr_height);
        Move(p_window->RPort,TEXT_AREA1_X+14*8,TEXT_AREA1_Y+3*8);
        Text(p_window->RPort,buf,4);
    }
    if (texts & TEXT_ADDRESS) {
        sprintf(buf,"%06lx",p_cfg->pal_ptr);
        Move(p_window->RPort,TEXT_AREA1_X+12*8,TEXT_AREA1_Y+4*8);
        Text(p_window->RPort,buf,6);
    }

    /* Area 2 */
    if (texts & TEXT_RGB) {
        sprintf(buf,"%02d",p_cfg->rgb_idx);
        Move(p_window->RPort,TEXT_AREA2_X+6*8,TEXT_AREA2_Y+0*8);
        Text(p_window->RPort,buf,2);
        sprintf(buf,"%03x",p_cfg->pal[p_cfg->rgb_idx]);
        Move(p_window->RPort,TEXT_AREA2_X+10*8,TEXT_AREA2_Y+0*8);
        Text(p_window->RPort,buf,3);
    }
    if (texts & TEXT_MODEVEN) {
        sprintf(buf,"%+4d",p_cfg->mod_even);
        Move(p_window->RPort,TEXT_AREA2_X+9*8,TEXT_AREA2_Y+1*8);
        Text(p_window->RPort,buf,4);
    }
    if (texts & TEXT_MODODD) {
        sprintf(buf,"%+4d",p_cfg->mod_odd);
        Move(p_window->RPort,TEXT_AREA2_X+9*8,TEXT_AREA2_Y+2*8);
        Text(p_window->RPort,buf,4);
    }
    if (texts & TEXT_DISPLAY) {
        Move(p_window->RPort,TEXT_AREA2_X+9*8,TEXT_AREA2_Y+3*8);
        Text(p_window->RPort,s_text_disp[p_cfg->display],4);
    }
    if (texts & TEXT_PRIORITY) {
        Move(p_window->RPort,TEXT_AREA2_X+9*8,TEXT_AREA2_Y+4*8);
        Text(p_window->RPort,s_text_prio[p_cfg->priority],4);
    }
    if (texts & TEXT_SAVEAS) {
        Move(p_window->RPort,TEXT_AREA2_X+9*8,TEXT_AREA2_Y+5*8);
        Text(p_window->RPort,s_text_save[p_cfg->save_as],4);
    }

    /* Area 3 */
    if (texts & TEXT_BPLANES) {
        /* num planes */
        Move(p_window->RPort,TEXT_AREA3_X+12*8,TEXT_AREA3_Y+0*8);
        sprintf(buf,"%d",p_cfg->num_bpl);
        Text(p_window->RPort,buf,1);

        /* on/off info */
        y = TEXT_AREA3_Y+1*8;
        for (n = 0; n < 6; n++) {
            Move(p_window->RPort,TEXT_AREA3_X+2*8,y);
            x =  n < p_cfg->num_bpl ? 0 : 1;
            y += 8;
            Text(p_window->RPort,s_text_ooff[x],3);
        }
    }
    if (texts & TEXT_RESO) {
        /* lores or hires */
        n = p_cfg->flags & MODE_HIRES ? 1 : 0;
        Move(p_window->RPort,TEXT_AREA3_X+14*8,TEXT_AREA3_Y+0*8);
        Text(p_window->RPort,s_text_reso[n],5);
    }
    if (texts & TEXT_BPLLOCKS) {
        /* bpl locks */
        y = TEXT_AREA3_Y+1*8;
        x = p_cfg->bpl_lock;
        for (n = 0; n < 6; n++) {
            Move(p_window->RPort,TEXT_AREA3_X+6*8,y);
            Text(p_window->RPort,s_text_lock[x&1],1);
            y += 8; x >>= 1;
        }
    }
    if (texts & TEXT_BPLADDRS) {
        /* bpl locks and addresses */
        y = TEXT_AREA3_Y + 1*8;

        for (n = 0; n < 6; n++) {
            Move(p_window->RPort,TEXT_AREA3_X+9*8,y);
            sprintf(buf,"%06lx",p_cfg->bpl_ptr[n]);
            Text(p_window->RPort,buf,6);
            y += 8;
        }
    }

    /* Area2 and status line */

    SetAPen(p_window->RPort,3);

    if (texts & TEXT_STATUS) {
        Move(p_window->RPort,TEXT_AREA2_X,TEXT_AREA2_Y+6*8);
        
        if (p_cfg->ioerr == 0) {
            /* if no error.. print a status line */
            Text(p_window->RPort,s_status_lines[p_cfg->status],13);
        } else {
            sprintf(buf,"IoErr: %6ld",p_cfg->ioerr);
            Text(p_window->RPort,buf,13);
        }
    }
}

void update_pal_texts(struct Window *p_window, BOOL on)
{
    char buf[] = "   ";
    int x,y,n;
    n = 0; x = 0; y = 0;

    SetDrMd(p_window->RPort,JAM2);
    SetAPen(p_window->RPort,3);
    SetBPen(p_window->RPort,0);

    while (n < 32) {
        if (on) {
          sprintf(buf,"%02d",n);
        }        

        if (x == 7) {
            x = 0; ++y;
        }

        Move(p_window->RPort,TEXT_AREA0_X+x*28,TEXT_AREA0_Y+y*11);
        Text(p_window->RPort,buf,2);
        ++n; ++x;
    }
}


ULONG init_cop_cfg(cop_cfg_t *p_cfg, BOOL pal_only)
{
    int n;

    p_cfg->pal_ptr = 0;

    for (n = 0; n < 32+3; n++) {
        p_cfg->pal[n] = s_def_pal[n];
    }

    if (pal_only) {
        return RGB_CHANGE|TEXT_ADDRESS;
    }

    for (n = 0; n < 6; n++) {
        p_cfg->bpl_ptr[n] = 0x4000;
    }

    p_cfg->max_chipmem = SysBase->MaxLocMem & 0xffff0000;

    p_cfg->mod_even = 0;
    p_cfg->mod_odd = 0;

    p_cfg->scr_width = 320;
    p_cfg->scr_height = 200;

    p_cfg->display  = IDX_DISP_STD;
    p_cfg->priority = IDX_PRIO_PF1;
    p_cfg->save_as  = IDX_SAVE_IFF;

    p_cfg->pf1shft = 15;
    p_cfg->pf2shft = 15;
    p_cfg->num_bpl = 5;
    p_cfg->flags = MODE_STD;
    p_cfg->bpl_lock = 0x3f; /* all six planes */

    p_cfg->rgb_idx = 0;

    p_cfg->ioerr = 0;
    p_cfg->status = IDX_STATUS_VERSION;

    /* update everything */
    return ~0;
}
