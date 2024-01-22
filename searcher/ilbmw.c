/*-----------------------------------------------------------------------
 * ILBMW.C  Support routines for writing ILBM files using IFFParse.
 * (IFF is Interchange Format File.)
 *
 * Based on code by Jerry Morrison and Steve Shaw, Electronic Arts.
 * This software is in the public domain.
 *
 * This version for the Amiga computer.
 *
 * Heavily modified/simplified by Jouni 'Mr.Spiv' Korhonen.
 *
 *----------------------------------------------------------------------*/

#include <graphics/view.h>
#include "searcher.h"
#include "iffhandle.h"
#include "ilbm.h"


long save_iff(IFFHandle_t *iff, cop_cfg_t *cfg, BOOL odd)
{
    int ncolors;
    ULONG modeid = 0;
    UWORD  *tabw;
    ColorRegister_t cmapReg;
    int32_t n = 0, m, h;
    LONG form_size = 0;
    LONG body_size = 0;
    UBYTE *planes[MAXSAVEDEPTH];
    LONG bytes_per_row = cfg->scr_width >> 3;
    BitMapHeader_t bmhd;
    int nbplanes = cfg->num_bpl;

    if (nbplanes < 6) {
        ncolors = 1 << cfg->num_bpl;
    } else if (nbplanes > 6) {
        ncolors = 16;
        nbplanes = 6;
    } else {
        ncolors = 32;
    }
    if (cfg->flags & MODE_STD) {
        if (nbplanes == 6) {
            modeid |= EXTRA_HALFBRITE;
        }
    }
    if (cfg->flags & MODE_HIRES) {
        modeid |= HIRES;
    }
    if (cfg->flags & MODE_HAM) {
        modeid |= HAM;
        nbplanes = 6;
    }

    /* This is a bit odd stuff to save either the odd or even
     * bitplanes of a dual playfield picture. The number of
     * bitplanes and colors need to be adjusted accordingly.
     * Note that even if nbplanes is scales down the real number
     * of bitplanes is still kept in cfg->num_bpl.
     */
    if (cfg->flags & MODE_DUALPF && cfg->num_bpl > 1) {
        nbplanes = (nbplanes & 1) + (nbplanes >> 1);

        if (!odd) {
            nbplanes = cfg->num_bpl - nbplanes;
        }

        ncolors = 1 << nbplanes;
        /* undo modeid if HAM or HALFBRITE */
        modeid = modeid & ~(HAM | EXTRA_HALFBRITE);
    }

    bmhd.w = cfg->scr_width;
    bmhd.h = cfg->scr_height;
    bmhd.x = bmhd.y = 0;      /* Default position is (0,0).*/
    bmhd.nPlanes = nbplanes;
    bmhd.masking = mskNone;    //masking;
    bmhd.compression = cmpNone;
    bmhd.reserved1 = 0;
    /* transparent color hardcoded to palette entry 0 */
    bmhd.transparentColor = 0; //transparentColor;
    bmhd.pageWidth = cfg->scr_width;
    bmhd.pageHeight = cfg->scr_height;

    /* hard coded to PAL */
    bmhd.xAspect =  16;    /* 320:200 = 1.6 = 16:10 */
    bmhd.yAspect =  10;

    if (cfg->flags & MODE_HIRES) {
        bmhd.xAspect = bmhd.xAspect << 1;
    }

    /* calculate the total IFF file size, excluding FORM..
     * No compression so that we know BODY size in advance
     */
    form_size =    
        4 +                                     /* ID_ILBM  */
        4 + 4 + sizeof(BitMapHeader_t) +        /* BMHD + size + content */
        4 + 4 + ncolors*sizeofColorRegister +   /* CMAP + size + content */
        4 + 4 + 4;                              /* CMAG + size + content */

    /* add body size.. all source bitmaps are divisible by 16 pixels */
    body_size = bmhd.h * nbplanes * bytes_per_row;
    form_size = form_size + 4 + 4 + body_size;

    /* Start with FORM and unknown length */
    if ((n  = iff->putnum(iff,ID_FORM,4)) < 0) {
        return n;
    }
    if ((n = iff->putnum(iff,form_size,4)) < 0) {
        return n;
    }

    /* ILDM.BMHD */
    if ((n = iff->putnum(iff,ID_ILBM,4)) < 0) {
        return n;
    }

    /* BMHD */
    if ((n = iff->putnum(iff,ID_BMHD,4)) < 0) {
        return n;
    }
    if ((n = iff->putnum(iff,sizeof(BitMapHeader_t),4)) < 0) {
        return n;
    }
    if ((n = iff->putnum(iff,bmhd.w,2)) < 0) {
        return n;
    }
    if ((n = iff->putnum(iff,bmhd.h,2)) < 0) {
        return n;
    }
    if ((n = iff->putnum(iff,bmhd.x,2)) < 0) {
        return n;
    }
    if ((n = iff->putnum(iff,bmhd.y,2)) < 0) {
        return n;
    }
    if ((n = iff->putc(iff,bmhd.nPlanes)) < 0) {
        return n;
    }
    if ((n = iff->putc(iff,bmhd.masking)) < 0) {
        return n;
    }
    if ((n = iff->putc(iff,bmhd.compression)) < 0) {
        return n;
    }
    if ((n = iff->putc(iff,bmhd.reserved1)) < 0) {
        return n;
    }
    if ((n = iff->putnum(iff,bmhd.transparentColor,2)) < 0) {
        return n;
    }
    if ((n = iff->putc(iff,bmhd.xAspect)) < 0) {
        return n;
    }
    if ((n = iff->putc(iff,bmhd.yAspect)) < 0) {
        return n;
    }
    if ((n = iff->putnum(iff,bmhd.pageWidth,2)) < 0) {
        return n;
    }
    if ((n = iff->putnum(iff,bmhd.pageHeight,2)) < 0) {
        return n;
    }

    /* size of CMAP is 3 bytes * ncolors */
    if ((n = iff->putnum(iff,ID_CMAP,4)) < 0) {
        return n;
    }
    if ((n = iff->putnum(iff,ncolors*sizeofColorRegister,4)) < 0) {
        return n;
    }

    /* Store each 4-bit value n as nn */
    if (cfg->flags & MODE_DUALPF && cfg->num_bpl > 1 && !odd) {
        /* In case of dual playfield even colors start from the
         * palette entry #8:
         */
        tabw = &cfg->pal[8];
    } else {
        tabw = cfg->pal;
    }

    for( ;  ncolors;  --ncolors ) {
        /* Red */
        cmapReg.red    = ( *tabw >> 4 ) & 0xf0;
        cmapReg.red   |= (cmapReg.red >> 4);

        cmapReg.green  = ( *tabw      ) & 0xf0;
        cmapReg.green |= (cmapReg.green >> 4);

        cmapReg.blue   = ( *tabw << 4 ) & 0xf0;
        cmapReg.blue  |= (cmapReg.blue >> 4);

        if ((n = iff->write(iff,&cmapReg,sizeofColorRegister)) < 0) {
            return n;
        }

        ++tabw;
    }

    /* put CAMG */
    if (modeid) {
        if ((n = iff->putnum(iff,ID_CAMG,4)) < 0) {
            return n;
        }
        if ((n = iff->putnum(iff,4,4)) < 0) {
            return n;
        }
        if ((n = iff->putnum(iff,modeid,4)) < 0) {
            return n;
        }
    }

    /* put BODY */
    if ((n = iff->putnum(iff,ID_BODY,4)) < 0) {
        return n;
    }
    if ((n = iff->putnum(iff,body_size,4)) < 0) {
        return n;
    }

    /* this might be a bit complicated scatter saving..
     * We use cfg->num_bpl since in case of dual playfield
     * the nbplanes is the number of PF1 or PF2 planes, not
     * the total number of bitplanes in the captured picture.
     */
    for (m = 0; m < cfg->num_bpl; m++) {
        planes[m] = (UBYTE *)cfg->bpl_ptr[m];
    }

    for (h = 0; h < bmhd.h; h++) {
        /* save one row of all planes */
        for (m = 0; m < cfg->num_bpl; m++) {
            if (!(cfg->flags & MODE_DUALPF) ||
               (!odd && (m & 1)) ||
               (odd && (!(m & 1)))) {

                if ((n = iff->write(iff,planes[m],bytes_per_row)) < 0) {
                    return -1;
                }
            }
            /* go to next lines.. screen width + modulo */
            planes[m] += bytes_per_row;

            if (m & 1) {
                /* Even bit plane */
                planes[m] += cfg->mod_even;
            } else {
                /* Odd bit plane */
                planes[m] += cfg->mod_odd;
            }
        }
    }

    return form_size+8;
}

long save_raw(IFFHandle_t *iff, cop_cfg_t *cfg, BOOL odd)
{
    int32_t n, m, h;
    UBYTE *planes[MAXSAVEDEPTH];
    LONG bytes_per_row = cfg->scr_width >> 3;
    LONG body_size;
    int nbplanes = cfg->num_bpl;

    if (nbplanes > 6) {
        nbplanes = 6;
    }
    if (cfg->flags & MODE_HAM) {
        nbplanes = 6;
    }

    body_size = cfg->scr_height * nbplanes * bytes_per_row;

    for (m = 0; m < nbplanes; m++) {
        planes[m] = (UBYTE *)cfg->bpl_ptr[m];
    }

    for (h = 0; h < cfg->scr_height; h++) {
        /* save one row of all planes */
        for (m = 0; m < cfg->num_bpl; m++) {
            if (!(cfg->flags & MODE_DUALPF) ||
               (!odd && (m & 1)) ||
               (odd && (!(m & 1)))) {

                if ((n = iff->write(iff,planes[m],bytes_per_row)) < 0) {
                    return n;
                }
            }
            /* go to next lines.. screen width + modulo */
            planes[m] += bytes_per_row;

            if (m & 1) {
                /* Even bit plane */
                planes[m] += cfg->mod_even;
            } else {
                /* Odd bit plane */
                planes[m] += cfg->mod_odd;
            }
        }
    }

    return body_size;
}

long save_pal(IFFHandle_t *iff, cop_cfg_t *cfg, BOOL odd)
{
    int nbplanes = cfg->num_bpl;
    int ncolors;
    int off = 0;

    if (nbplanes < 6) {
        ncolors = 1 << cfg->num_bpl;
    } else if (nbplanes > 6) {
        ncolors = 16;
    } else {
        ncolors = 32;
    }
    if (cfg->flags & MODE_DUALPF && cfg->num_bpl > 1) {
        nbplanes = (nbplanes & 1) + (nbplanes >> 1);

        if (!odd) {
            nbplanes = cfg->num_bpl - nbplanes;
            off = 8;
        }

        ncolors = 1 << nbplanes;
    }

    return iff->write(iff,&cfg->pal[off],ncolors*2);
}
