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
#include <proto/dos.h>

#include <exec/execbase.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <dos/stdio.h>
#include <dos/dos.h>
#include <graphics/copper.h>
#include <graphics/text.h>
#include <graphics/sprite.h>
#include <graphics/view.h>
#include <hardware/custom.h>

#include <clib/alib_stdio_protos.h>
#include <clib/macros.h>
#include <string.h>

#include "searcher.h"
#include "ilbm.h"
#include "colfind.h"
#include "screen.h"

static const char s_ver[] = "$VER: Graphics Searcher v"MSTR(MAJOR)"."MSTR(MINOR)" (21.1.2024) by Jouni 'Mr.Spiv/Scoopex' Korhonen";


/* Use the following to compile:
 *  vc +kick13m -lamiga -sd -sc -O2 -c99 searcher.c  -o gs.exe
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

static UBYTE s_file_name_buf[MAX_FILENAME];
static UBYTE s_file_undo_buf[MAX_FILENAME];

/* my gadget for inputting filename.. */

static UWORD s_strBorderData[] = {
    0,0, GAD_STR_W + 3,0, GAD_STR_W + 3,GAD_STR_H + 3,
    0,GAD_STR_H + 3, 0,0
};

static struct Border s_strBorder = {
    -2,-2,1,0,JAM2,5,s_strBorderData,NULL
};

struct StringInfo s_strInfo = {
    s_file_name_buf,
    s_file_undo_buf,
    0,
    MAX_FILENAME,
    0,
    0,0,0,0,0,
    NULL,0,NULL
};

struct Gadget s_strGadget = {
    NULL, 204,50,GAD_STR_W,GAD_STR_H,
    GFLG_GADGHCOMP|GFLG_DISABLED, GACT_RELVERIFY,
    GTYP_STRGADGET, &s_strBorder, NULL, NULL,0,&s_strInfo,0,NULL,
};


/* default palette and stuff */

static UWORD s_head_palette[] = {0x0203, 0x0666, 0x0102, 0x0f66};


static struct TagItem s_screen_tags[] = {
    {SA_Draggable,FALSE},
    {TAG_DONE,0}
};


static struct ExtNewScreen s_newscreen = {
    0,0,                                        /* LeftEdge, TopEdge */
    640,63,2,                                   /* Width, Height, Depth */
    0,1,                                        /* DetailPen, BlockPen */
    HIRES|SPRITES,                              /* ViewModes */
    CUSTOMSCREEN|SCREENQUIET,                   /* Type */
    &s_topaz,                                   /* Font */
    NULL,
    NULL,                                       /* *Gadgets */
    NULL,                                       /* *CustomBitmap */
    s_screen_tags                               /* *Extension */
};


static const struct ExtNewWindow s_newwindow = {
    0,0,                                        /* LeftEdge, TopEdge */
    640,63,                                     /* Width, Height */
    JAM1,JAM2,                                  /* DetailPen, BlockPen */
    IDCMP_RAWKEY|IDCMP_GADGETUP,                /* IDCMPFlags */
    WFLG_BORDERLESS|WFLG_RMBTRAP|WFLG_ACTIVATE, /* Flags */
    &s_strGadget,                               /* *FirstGadget */
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

/* Function prototypes.. unneeded but still here */
static BOOL open_screen_window(void);
static void close_screen_window(void);
static ULONG inc_scr_width(cop_cfg_t *p_cfg);
static ULONG dec_scr_width(cop_cfg_t *p_cfg);
static ULONG reset_bplane_addrs(cop_cfg_t *p_cfg, LONG ptr);
static ULONG set_bplane_lock(cop_cfg_t *p_cfg, int bpl_idx);
static ULONG handle_display_mode(cop_cfg_t *p_cfg);
static ULONG handle_save_as(cop_cfg_t *p_cfg);
static ULONG set_bplane_height(cop_cfg_t *p_cfg, WORD height);
static ULONG handle_bplane_height(cop_cfg_t *p_cfg, int qual);
static ULONG dec_bplane_num(cop_cfg_t *p_cfg);
static ULONG inc_bplane_num(cop_cfg_t *p_cfg);
static ULONG handle_bplane_move(cop_cfg_t *p_cfg, int key, int qual);
static ULONG set_resolution(cop_cfg_t *p_cfg, BOOL hires);
static ULONG handle_palette(cop_cfg_t *p_cfg, int key);
static ULONG dec_palette_index(cop_cfg_t *p_cfg);
static ULONG inc_palette_index(cop_cfg_t *p_cfg);
static ULONG set_chipmem_addr(cop_cfg_t *p_cfg, int key);
static ULONG handle_modulos(cop_cfg_t *p_cfg, int key, int qual);
static ULONG handle_priority(cop_cfg_t *p_cfg);
static ULONG handle_filename(cop_cfg_t *p_cfg);
static int handle_filesave(cop_cfg_t *p_cfg, struct Gadget *p_gad);
static BOOL main_key_loop(int key, int qual, cop_cfg_t *p_cfg);
static ULONG handle_palette_move(cop_cfg_t *p_cfg, int qual, int step);
static ULONG handle_copper_search(cop_cfg_t *p_cfg, int qual, int min_num);


/* Spaghetti code starts.. there are hardly comments.. */

static BOOL open_screen_window(void)
{
    /* check for V39.. patch then tag extension */
    if (SysBase->LibNode.lib_Version >= 39) {
        s_newscreen.Type |= NS_EXTENDED;
    }

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


static void close_screen_window(void)
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


static ULONG inc_scr_width(cop_cfg_t *p_cfg)
{
    WORD new_width = p_cfg->scr_width;

    if (p_cfg->flags & MODE_HIRES) {
        new_width += 32;
    } else {
        new_width += 16;
    }

    /* The picture can be a lot bigger than the visible screen.
     * Larger than visible screen picture act as there was modulo
     * added but in practise the real difference is when saving the
     * picture.. when saving modulos are not part of the saved picture.
     * With this kludge it is possible tosave like 1000 pixels wide
     * pictures.
     */
    p_cfg->scr_width = new_width < MAX_BPLANE_WIDTH ? new_width : MAX_BPLANE_WIDTH;
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


static ULONG reset_bplane_addrs(cop_cfg_t *p_cfg, LONG ptr)
{
    p_cfg->bpl_ptr[0] = ptr;
    p_cfg->bpl_ptr[1] = ptr;
    p_cfg->bpl_ptr[2] = ptr;
    p_cfg->bpl_ptr[3] = ptr;
    p_cfg->bpl_ptr[4] = ptr;
    p_cfg->bpl_ptr[5] = ptr;
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
    ULONG ret = TEXT_DISPLAY;

    if (++p_cfg->display > IDX_DISP_MAX) {
        p_cfg->display = 0;
    }

    p_cfg->flags = p_cfg->flags & ~(MODE_HAM|MODE_STD|MODE_DUALPF);
    p_cfg->flags = p_cfg->flags | (1 << p_cfg->display);

    /* Need to check if HAM is set and hires is set
     * Do not reset screen width, though..
     */

    if ((p_cfg->display == IDX_DISP_HAM) && (p_cfg->flags & MODE_HIRES)) {
        p_cfg->flags &= ~MODE_HIRES;
        ret |= TEXT_RESO;
    }

    return ret; 
}


static ULONG handle_save_as(cop_cfg_t *p_cfg)
{
    if (++p_cfg->save_as > IDX_SAVE_MAX) {
        p_cfg->save_as = 0;
    }
    return TEXT_SAVEAS;
}


static ULONG set_bplane_height(cop_cfg_t *p_cfg, WORD height)
{
    p_cfg->scr_height = height;
    return TEXT_HEIGHT;
}


static ULONG handle_bplane_height(cop_cfg_t *p_cfg, int qual)
{
    WORD height = p_cfg->scr_height;
    WORD step=1;

    if (qual & IEQUALIFIER_LSHIFT) {
        step = -1;
    }
    if (height + step > 0 && height + step < 9999) {
        height += step;
    }

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
        step_even  = 2;
        step_odd   = 2;
        break;
    case KEY_RIGHT:
        step_even  = -2;
        step_odd   = -2;
        break;
    case KEY_DOWN:
        step_even = -((p_cfg->scr_width >> 3) + p_cfg->mod_even);
        step_odd  = -((p_cfg->scr_width >> 3) + p_cfg->mod_odd);
        break;
    case KEY_UP:
        step_even = (p_cfg->scr_width >> 3) + p_cfg->mod_even;
        step_odd  = (p_cfg->scr_width >> 3) + p_cfg->mod_odd;
        break;
    default:
        return 0;
    }

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
                p_cfg->bpl_ptr[n] = (p_cfg->bpl_ptr[n] + (step_even * m)) & (p_cfg->max_chipmem-1);
            } else {
                /* odd bitplane */
                p_cfg->bpl_ptr[n] = (p_cfg->bpl_ptr[n] + (step_odd * m)) & (p_cfg->max_chipmem-1);
            }
        }
    }

    return TEXT_BPLADDRS;
}


static ULONG set_resolution(cop_cfg_t *p_cfg, BOOL hires)
{
    ULONG ret = TEXT_RESO|TEXT_SCREEN|TEXT_PF1WIDTH|TEXT_PF2WIDTH;

    if (hires) {
        p_cfg->flags |= MODE_HIRES;
        p_cfg->scr_width = 640;
        
        if (p_cfg->num_bpl > 4) {
            p_cfg->num_bpl = 4;
            ret |= TEXT_BPLANES;
        }
        if (p_cfg->flags & MODE_HAM) {
            p_cfg->flags &= ~MODE_HAM;
            p_cfg->display = IDX_DISP_STD;
            ret |= TEXT_DISPLAY;
        }
    } else {
        p_cfg->flags &= ~MODE_HIRES;
        p_cfg->scr_width = 320;
    }
    return ret;
}


static ULONG handle_palette(cop_cfg_t *p_cfg, int key)
{
    UWORD *p_rgb = &p_cfg->pal[p_cfg->rgb_idx];
    UWORD inc=0,dec=0,msk=0;
    UWORD rgb = *p_rgb;

    switch (key) {
    case KEY_NP_0:
        if (rgb == 0xfff || rgb == 0) {
            *p_rgb = rgb ^ 0x0fff;
        } else {
            *p_rgb = 0;
        } 
        return TEXT_RGB;

    case KEY_NP_7:
        inc = 0x100;
        msk = 0xf00;
        break;
    case KEY_NP_4:
        dec = 0x100;
    case KEY_NP_1:
        msk = 0xf00;
        break;

    case KEY_NP_8:
        inc = 0x010;
        msk = 0x0f0;
        break;
    case KEY_NP_5:
        dec = 0x010;
    case KEY_NP_2:
        msk = 0x0f0;
        break;

    case KEY_NP_9:
        inc = 0x001;
        msk = 0x00f;
        break;
    case KEY_NP_6:
        dec = 0x001;
    case KEY_NP_3:
        msk = 0x00f;
        break;

    default:
        return 0;
    }

    if (inc > 0) {
        if ((rgb & msk) < msk) {
            rgb = ((rgb & msk) + inc) | (rgb & ~msk);
        }
    }
    if (dec > 0) {
        if ((rgb & msk) > 0) {
            rgb = ((rgb & msk) - dec) | (rgb & ~msk);
        }
    }
    if (inc == 0 && dec == 0) {
        if ((rgb & msk) == msk || (rgb & msk) == 0) {
            rgb = rgb ^ msk;
        } else {
            rgb = rgb & ~msk;
        } 
    }

    *p_rgb = rgb;
    return TEXT_RGB;
}


static ULONG dec_palette_index(cop_cfg_t *p_cfg)
{
    p_cfg->rgb_idx = (p_cfg->rgb_idx-1) & 0x1f;
    return TEXT_RGB;
}


static ULONG inc_palette_index(cop_cfg_t *p_cfg)
{
    p_cfg->rgb_idx = (p_cfg->rgb_idx+1) & 0x1f;
    return TEXT_RGB;
}


static ULONG set_chipmem_addr(cop_cfg_t *p_cfg, int key)
{
    LONG ptr = 0;
    int n;

    switch (key) {
    case KEY_F8:
        switch (p_cfg->max_chipmem >> 16) {
        case 0x4:   /* 256K CHIP */
        case 0x8:   /* 512K CHIP */
            ptr = 0x00000;
            break;        
        case 0x10:  /* 1M CHIP */
            ptr = 0x80000;
            break;
        case 0x20:  /* 2M CHIP */
            ptr = 0x100000;
            break;
        default:
            return 0;
        }
        break;
    case KEY_F9:
        switch (p_cfg->max_chipmem >> 16) {
        case 0x4:   /* 256K CHIP */
            ptr = 0x20000;
        case 0x8:   /* 512K CHIP */
            ptr = 0x50000;
            break;        
        case 0x10:  /* 1M CHIP */
            ptr = 0xd0000;
            break;
        case 0x20:  /* 2M CHIP */
            ptr = 0x150000;
            break;
        default:
            return 0;
        }
        break;   
    default:
        return 0;
    }

    for (n = 0; n < 6; n++) {
        p_cfg->bpl_ptr[n] = ptr;
    }

    return TEXT_BPLADDRS;
}


static ULONG handle_modulos(cop_cfg_t *p_cfg, int key, int qual)
{
    WORD step_odd=0, step_even=0;
    int n;

    switch (key) {
    case KEY_FSTOP:
        step_even  = 2;
        step_odd   = 2;
        break;
    case KEY_COMMA:
        step_even  = -2;
        step_odd   = -2;
        break;
    case KEY_M:
        step_even  = 10;
        step_odd   = 10;
        break;
    case KEY_N:
        step_even  = -10;
        step_odd   = -10;
        break;
    case KEY_SLASH:
        if (qual & IEQUALIFIER_RSHIFT) {
            p_cfg->mod_even = 0;
            return TEXT_MODEVEN;
        } else if (qual & IEQUALIFIER_LSHIFT) {
            p_cfg->mod_odd  = 0;
            return TEXT_MODODD;
        } else {
            p_cfg->mod_even = 0;
            p_cfg->mod_odd  = 0;
            return TEXT_MODEVEN|TEXT_MODODD;
        }
        break;
    default:
        return 0;
    }

    if (qual & IEQUALIFIER_RSHIFT) {
        if (ABS(p_cfg->mod_even + step_even) < MAX_MODULO ) {
            p_cfg->mod_even = p_cfg->mod_even + step_even;
        }
        return TEXT_MODEVEN;
    } else if (qual & IEQUALIFIER_LSHIFT) {
        if (ABS(p_cfg->mod_odd + step_odd) < MAX_MODULO) {
            p_cfg->mod_odd = p_cfg->mod_odd + step_odd;
        }
        return TEXT_MODODD;
    }
    if (ABS(p_cfg->mod_odd + step_odd) < MAX_MODULO) {
        p_cfg->mod_odd  = p_cfg->mod_odd + step_odd;
    }
    if (ABS(p_cfg->mod_even + step_even) < MAX_MODULO) {
        p_cfg->mod_even = p_cfg->mod_even + step_even;
    }
    return TEXT_MODODD|TEXT_MODEVEN;
}


static ULONG handle_priority(cop_cfg_t *p_cfg)
{
    if (++p_cfg->priority > IDX_PRIO_MAX) {
        p_cfg->priority = 0;
        p_cfg->flags &= ~MODE_PF2PRI;
    } else {
        p_cfg->flags |= MODE_PF2PRI;
    }

    return TEXT_PRIORITY;
}


static ULONG handle_filename(cop_cfg_t *p_cfg)
{
    OnGadget(&s_strGadget,sp_window,NULL);
    return 0;
}


static int handle_filesave(cop_cfg_t *p_cfg, struct Gadget *p_gad)
{
    IFFHandle_t* fh;
    int ret = -1;
    char *tmp_name = ((struct StringInfo*)p_gad->SpecialInfo)->Buffer;
    char *postfix = "";
    char name[256];

    OffGadget(p_gad,sp_window,NULL);

    if (strlen(tmp_name) <= 0) {
        p_cfg->ioerr = 0;
        p_cfg->status = IDX_STATUS_NOSAVE;
        update_head_texts(sp_window,FALSE,TEXT_STATUS,p_cfg);
        return -1;
    }

    p_cfg->status = IDX_STATUS_SAVEOK;
    p_cfg->ioerr = 0;

    if ((fh = initIFFHandle()) == NULL) {
        p_cfg->ioerr = 0;
        p_cfg->status = IDX_STATUS_NORAM;
        update_head_texts(sp_window,FALSE,TEXT_STATUS,p_cfg);
        return -1;        
    }
    if (p_cfg->flags & MODE_DUALPF) {
        postfix = "_pf1";
    }

    sprintf(name,"%s%s",tmp_name,postfix);

    if (fh->open(fh,name,MODE_NEWFILE)) {
        switch (p_cfg->save_as) {
        case IDX_SAVE_IFF:
            ret = save_iff(fh,p_cfg,TRUE);
            break;
        case IDX_SAVE_RAW:
            ret = save_raw(fh,p_cfg,TRUE);
            break;
        case IDX_SAVE_PAL:
            ret = save_pal(fh,p_cfg,TRUE);
            break;
        default:
            p_cfg->status = IDX_STATUS_FAULT;
            break;
        }

        fh->close(fh);
    }

    if (ret < 0) {
        p_cfg->ioerr = fh->ioerr(fh);
        freeIFFHandle(fh);
        update_head_texts(sp_window,FALSE,TEXT_STATUS,p_cfg);
        return -1;
    }

    ret = -1;

    if (p_cfg->flags & MODE_DUALPF) {
        sprintf(name,"%s_pf2",tmp_name);

        if (fh->open(fh,name,MODE_NEWFILE)) {
            switch (p_cfg->save_as) {
            case IDX_SAVE_IFF:
                ret = save_iff(fh,p_cfg,FALSE);
                break;
            case IDX_SAVE_RAW:
                ret = save_raw(fh,p_cfg,FALSE);
                break;
            case IDX_SAVE_PAL:
                ret = save_pal(fh,p_cfg,FALSE);
                break;
            default:
                p_cfg->status = IDX_STATUS_FAULT;
                break;
            }

            if (ret < 0) {
                p_cfg->ioerr = fh->ioerr(fh);
            }

            fh->close(fh);
        }
    }

    freeIFFHandle(fh);
    update_head_texts(sp_window,FALSE,TEXT_STATUS,p_cfg);
    return -1;
}


static ULONG handle_palette_move(cop_cfg_t *p_cfg, int qual, int step)
{
    UWORD tmp;
    int n;

    LONG   pp = p_cfg->pal_ptr;
    LONG mask = p_cfg->max_chipmem - 1;
    LONG shft = 0;

    if (qual & IEQUALIFIER_LSHIFT) {
        step *= 2;
        shft = 2;
    }

    pp = (pp + step) & mask;
    p_cfg->pal_ptr = pp;
    step = ABS(step);

    for (n = 0; n < 32; n++) {
        p_cfg->pal[n] = *((UWORD*)((pp + shft) & mask));
        pp = (pp + step) & mask;
    }

    return RGB_CHANGE|TEXT_ADDRESS;
}


static ULONG handle_copper_search(cop_cfg_t *p_cfg, int qual, int min_num)
{
    return copper_search(p_cfg,qual,min_num);
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
        update_pal_texts(sp_window,onoff);
        onoff = !onoff;
        break;
    case KEY_NP_0: case KEY_NP_1: case KEY_NP_2: case KEY_NP_3: case KEY_NP_4:
    case KEY_NP_5: case KEY_NP_6: case KEY_NP_7: case KEY_NP_8: case KEY_NP_9:
        update = handle_palette(p_cfg,key);
        break;
    case KEY_NP_MINUS:
        update = dec_palette_index(p_cfg);
        break;
    case KEY_NP_ENTER:
        update = inc_palette_index(p_cfg);
        break;

    case KEY_1: case KEY_2: case KEY_3: case KEY_4: 
    case KEY_5: case KEY_6: case KEY_7: 
        update = set_bplane_lock(p_cfg,key-KEY_1);
        break;
    case KEY_8:
        update = reset_bplane_addrs(p_cfg,p_cfg->bpl_ptr[0]);
        break;
    case KEY_0:
        update = reset_bplane_addrs(p_cfg,0);
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

    case KEY_P:
        update = handle_priority(p_cfg);
        break;

    case KEY_D:
        update = init_cop_cfg(p_cfg,TRUE);
        break;

    case KEY_X:
        update = handle_copper_search(p_cfg,qual,4);
        break;
    case KEY_C:
        update = handle_copper_search(p_cfg,qual,8);
        break;

    case KEY_V:
        update = handle_palette_move(p_cfg,qual,-2);
        break;
    case KEY_B:
        update = handle_palette_move(p_cfg,qual,2);
        break;


    case KEY_UP:
    case KEY_DOWN:
    case KEY_LEFT:
    case KEY_RIGHT:
        update = handle_bplane_move(p_cfg,key,qual);
        break;

    case KEY_F6:
        update = handle_bplane_height(p_cfg,qual);
        break;
    case KEY_F7:
        update = handle_save_as(p_cfg);
        break;
    case KEY_F8: case KEY_F9:
        update = set_chipmem_addr(p_cfg,key);
        break;

    case KEY_N: case KEY_M: case KEY_COMMA:
    case KEY_FSTOP: case KEY_SLASH:
        update = handle_modulos(p_cfg,key,qual);
        break;

    case KEY_TAB:
        update = handle_filename(p_cfg);
        break;

    default:
        break;
    }

    if (update) {
        update_head_texts(sp_window,FALSE,update,p_cfg);
    }
    if (update & TEXT_COPPER_MASK) {
        setup_copper(sp_screen,p_cfg);
    }

    return FALSE;
}


int main(char *argv)
{
    struct IntuiMessage *p_msg;
    struct Gadget *p_gad;
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
        update_head_texts(sp_window,TRUE,~0,&cfg); 
        setup_sprites(sp_screen);
        p_ucl = setup_copper(sp_screen, &cfg);

        win_sig_bit = sp_window->UserPort->mp_SigBit;
        wait_mask = 1L << win_sig_bit;

        while (!done) {
            sig = Wait(wait_mask);

            if (sig & (1L << win_sig_bit)) {
                while (p_msg = (struct IntuiMessage *)GetMsg(sp_window->UserPort)) {
                    key = -1;
                    p_gad = NULL;

                    if (p_msg->Class == IDCMP_RAWKEY) {
                        key = p_msg->Code;
                        qual = p_msg->Qualifier;
                    }
                    if (p_msg->Class == IDCMP_GADGETUP) {
                        p_gad = (struct Gadget*)p_msg->IAddress;
                    }

                    ReplyMsg((struct Message *)p_msg);

                    if (p_gad) {
                        key = handle_filesave(&cfg,p_gad);
                    }
                    if (key >= 0) {
                        done = main_key_loop(key,qual,&cfg);
                    }
                }
            }
        }
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

