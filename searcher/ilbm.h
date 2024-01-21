/*
 *
 * ilbm.h:      Definitions for IFFParse ILBM reader.
 *
 * 6/27/91
 *
 * Heavily modified/simplified by Jouni 'Mr.Spiv' Korhonen.
 */

#ifndef IFFP_ILBM_H
#define IFFP_ILBM_H

#include <exec/types.h>
#include <dos/dos.h>
#include "iffhandle.h"

#define MAKE_ID(a,b,c,d) (a<<24|b<<16|c<<8|d)

/*  IFF types we may encounter  */
#define ID_FORM         MAKE_ID('F','O','R','M')
#define ID_ILBM         MAKE_ID('I','L','B','M')


/* ILBM Chunk ID's we may encounter
 * (see iffp/iff.h for some other generic chunks)
 */
#define ID_BMHD         MAKE_ID('B','M','H','D')
#define ID_CMAP         MAKE_ID('C','M','A','P')
#define ID_CRNG         MAKE_ID('C','R','N','G')
#define ID_CCRT         MAKE_ID('C','C','R','T')
#define ID_GRAB         MAKE_ID('G','R','A','B')
#define ID_SPRT         MAKE_ID('S','P','R','T')
#define ID_DEST         MAKE_ID('D','E','S','T')
#define ID_CAMG         MAKE_ID('C','A','M','G')
/* Generic Chunk ID's we may encounter */
#define ID_ANNO         MAKE_ID('A','N','N','O')
#define ID_AUTH         MAKE_ID('A','U','T','H')
#define ID_CHRS         MAKE_ID('C','H','R','S')
#define ID_Copyright    MAKE_ID('(','c',')',' ')
#define ID_CSET         MAKE_ID('C','S','E','T')
#define ID_FVER         MAKE_ID('F','V','E','R')
#define ID_NAME         MAKE_ID('N','A','M','E')
#define ID_TEXT         MAKE_ID('T','E','X','T')
#define ID_BODY         MAKE_ID('B','O','D','Y')


/* Use this constant instead of sizeof(ColorRegister). */
#define sizeofColorRegister  3

typedef WORD Color4;   /* Amiga RAM version of a color-register,
                        * with 4 bits each RGB in low 12 bits.*/

/* Maximum number of bitplanes storable in BitMap structure */
#define MAXAMDEPTH 8
#define MAXAMCOLORREG 32

/* Maximum planes we can save */
#define MAXSAVEDEPTH 6

/*  Masking techniques  */
#define mskNone                 0
#define mskHasMask              1
#define mskHasTransparentColor  2
#define mskLasso                3

/*  Compression techniques  */
#define cmpNone                 0
#define cmpByteRun1             1

/* ---------- BitMapHeader ---------------------------------------------*/
/*  Required Bitmap header (BMHD) structure describes an ILBM */
typedef struct BitMapHeader {
        UWORD   w, h;           /* Width, height in pixels */
        WORD    x, y;           /* x, y position for this bitmap  */
        UBYTE   nPlanes;        /* # of planes (not including mask) */
        UBYTE   masking;        /* a masking technique listed above */
        UBYTE   compression;    /* cmpNone or cmpByteRun1 */
        UBYTE   reserved1;      /* must be zero for now */
        UWORD   transparentColor;
        UBYTE   xAspect, yAspect;
        WORD    pageWidth, pageHeight;
} BitMapHeader_t;

/* ---------- ColorRegister --------------------------------------------*/
/* A CMAP chunk is a packed array of ColorRegisters (3 bytes each). */
typedef struct {
    UBYTE red, green, blue;   /* MUST be UBYTEs so ">> 4" won't sign extend.*/
} ColorRegister_t;

/* ---------- Camg Amiga Viewport Mode ---------------------------------*/
/* A Commodore Amiga ViewPort->Modes is stored in a CAMG chunk. */
/* The chunk's content is declared as a LONG. */
typedef struct {
   ULONG ViewModes;
} CamgChunk_t;

/* ilbmw.c  ILBM writer routines.. no compression.. just the simplest */

long save_iff(IFFHandle_t *iff, cop_cfg_t *cfg);
long save_raw(IFFHandle_t *iff, cop_cfg_t *cfg);
long save_pal(IFFHandle_t *iff, cop_cfg_t *cfg);

#endif /* IFFP_ILBM_H */
