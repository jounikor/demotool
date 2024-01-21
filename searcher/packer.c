/*----------------------------------------------------------------------*
 * packer.c Convert data to "cmpByteRun1" run compression.     11/15/85
 *
 * By Jerry Morrison and Steve Shaw, Electronic Arts.
 * This software is in the public domain.
 *
 *      control bytes:
 *       [0..127]   : followed by n+1 bytes of data.
 *       [-1..-127] : followed by byte to be repeated (-n)+1 times.
 *       -128       : NOOP.
 *
 * This version for the Amiga computer.
 *----------------------------------------------------------------------*/
#include "packer.h"

#define DUMP    0
#define RUN     1

#define MinRun 3
#define MaxRun 128
#define MaxDat 128

/* When used on global definitions, static means private.
 * This keeps these names, which are only referenced in this
 * module, from conficting with same-named objects in your program.
 */
static LONG putSize;
static char buf[130];   /* [TBD] should be 128?  on stack?*/

#define GetByte()       (*source++)
#define PutByte(c)      { *dest++ = (c);   ++putSize; }

static BYTE *PutDump(BYTE *dest, int nn) {
        int i;

        PutByte(nn-1);
        for(i = 0;  i < nn;  i++)   PutByte(buf[i]);
        return(dest);
        }

static BYTE *PutRun(BYTE *dest, int nn, int cc) {
        PutByte(-(nn-1));
        PutByte(cc);
        return(dest);
        }

#define OutDump(nn)   dest = PutDump(dest, nn)
#define OutRun(nn,cc) dest = PutRun(dest, nn, cc)

/*----------- packrow --------------------------------------------------*/
/* Given POINTERS TO POINTERS, packs one row, updating the source and
 * destination pointers.  RETURNs count of packed bytes.
 */
LONG packrow(BYTE **pSource, BYTE **pDest, LONG rowSize)
    {
    BYTE *source, *dest;
    char c,lastc = '\0';
    BOOL mode = DUMP;
    short nbuf = 0;             /* number of chars in buffer */
    short rstart = 0;           /* buffer index current run starts */

    source = *pSource;
    dest = *pDest;
    putSize = 0;
    buf[0] = lastc = c = GetByte();  /* so have valid lastc */
    nbuf = 1;   rowSize--;      /* since one byte eaten.*/


    for (;  rowSize;  --rowSize) {
        buf[nbuf++] = c = GetByte();
        switch (mode) {
                case DUMP:
                        /* If the buffer is full, write the length byte,
                           then the data */
                        if (nbuf>MaxDat) {
                                OutDump(nbuf-1);
                                buf[0] = c;
                                nbuf = 1;   rstart = 0;
                                break;
                                }

                        if (c == lastc) {
                            if (nbuf-rstart >= MinRun) {
                                if (rstart > 0) OutDump(rstart);
                                mode = RUN;
                                }
                            else if (rstart == 0)
                                mode = RUN;     /* no dump in progress,
                                so can't lose by making these 2 a run.*/
                            }
                        else  rstart = nbuf-1;          /* first of run */
                        break;

                case RUN: if ( (c != lastc)|| ( nbuf-rstart > MaxRun)) {
                        /* output run */
                        OutRun(nbuf-1-rstart,lastc);
                        buf[0] = c;
                        nbuf = 1; rstart = 0;
                        mode = DUMP;
                        }
                        break;
                }

        lastc = c;
        }

    switch (mode) {
        case DUMP: OutDump(nbuf); break;
        case RUN: OutRun(nbuf-rstart,lastc); break;
        }
    *pSource = source;
    *pDest = dest;
    return(putSize);
    }

