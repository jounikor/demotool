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

#include <proto/exec.h>
#define __NOLIBBASE__
#include <proto/dos.h>

#include <exec/execbase.h>
#include <exec/memory.h>
#include <dos/stdio.h>
#include <dos/dos.h>

#include <clib/alib_stdio_protos.h>
#include <clib/macros.h>
#include <string.h>
#include <stdint.h>

#include "iffhandle.h"
#include "searcher.h"

extern struct DosLibrary *DOSBase;

static int32_t s_open(IFFHandle_t *ctx, char* name, int32_t mode);
static int32_t s_close(IFFHandle_t *ctx);
static int32_t s_write(IFFHandle_t *ctx, void *buffer, int32_t length);
static int32_t s_putnum(IFFHandle_t *ctx, uint32_t, int32_t length);
static int32_t s_putc(IFFHandle_t *ctx, uint8_t ch);
static int32_t s_seek(IFFHandle_t *ctx, int32_t position, int32_t mode);
static int32_t s_ioerr(IFFHandle_t *ctx);
static int32_t s_flush(IFFHandle_t *ctx);

typedef struct  IFFHandle_ {
    int32_t (*open)(IFFHandle_t *ctx, char* name, int32_t mode);
    int32_t (*close)(IFFHandle_t *ctx);
    int32_t (*read)(IFFHandle_t *ctx, void *buffer, int32_t length);
    int32_t (*write)(IFFHandle_t *ctx, void *buffer, int32_t length);
    int32_t (*putnum)(IFFHandle_t *ctx, uint32_t num, int32_t length);
    int32_t (*putc)(IFFHandle_t *ctx, uint8_t ch);
    int32_t (*seek)(IFFHandle_t *ctx, int32_t position, int32_t mode);
    int32_t (*ioerr)(IFFHandle_t *ctx);
    int32_t (*flush)(IFFHandle_t *ctx);

    /* internal data.. decide based on your system */
    BPTR handle;
    LONG last_ioerr;
    LONG size;
    LONG buffer_size;
    LONG buffer_pos;
    UBYTE buffer[];
} IFFHandle_t_;

IFFHandle_t *initIFFHandle(void)
{
    IFFHandle_t_ *p_hndl;
    /* MAX_BPLANE_WIDTH comes from searcher.h */
    LONG size = sizeof(IFFHandle_t_) + 12*(MAX_BPLANE_WIDTH>>3);

    p_hndl = AllocMem(size,MEMF_PUBLIC|MEMF_CLEAR);

    if (p_hndl) {
        p_hndl->open = s_open;
        p_hndl->close = s_close;
        p_hndl->write = s_write;
        p_hndl->putnum = s_putnum;
        p_hndl->putc = s_putc;
        p_hndl->seek = s_seek;
        p_hndl->ioerr = s_ioerr;
        p_hndl->flush = s_flush;
        p_hndl->size = size;
        p_hndl->buffer_pos = 0;
        p_hndl->buffer_size = 12*(MAX_BPLANE_WIDTH>>3);
        p_hndl->handle = 0;
    }

#if 1
    ULONG *p_d = ((ULONG*)0x180000);
    ULONG *p_s = (ULONG*)p_hndl;

    *p_d++ = size;
    *p_d++ = p_hndl->buffer_size;
    *p_d++ = (ULONG)p_hndl;
    *p_d++ = (ULONG)&p_hndl->buffer[0];

    do {
        *p_d++ = *p_s++;
        size -= 4;
    } while (size > 0);



#endif



    return (struct IFFHandle*)p_hndl;
}

void freeIFFHandle(struct IFFHandle *p_hndl)
{
    IFFHandle_t_ *p = (IFFHandle_t_*)p_hndl;

    if (p) {
        if (p->handle) {
            p->close(p_hndl);
        }

        FreeMem(p,p->size);
    }
}

/* Amiga implementation.. I know this is a bit nuisance but done
 * here just do add some mow possible hard to find bugs and beef
 * up my already rusty programming skillz..
 */

static int32_t s_open(IFFHandle_t *ctx, char* name, int32_t mode)
{
    /* MODE_OLDFILE, MODE_NEWFILE, MODE_READWRITE */
    IFFHandle_t_ *p = (IFFHandle_t_*)ctx;
    
    if (p->handle) {
        /* gosh.. the handle is already in use */
        p->last_ioerr = ERROR_OBJECT_IN_USE;
        return 0;
    }
    if (mode == 0) {
        mode = MODE_NEWFILE;
    }

    /* we fo allow reuse of IFFHandle for several files */
    p->buffer_pos = 0;
    p->last_ioerr = 0;

    p->handle = Open(name,mode);
    return p->handle;
}

static int32_t s_close(IFFHandle_t *ctx) 
{
    IFFHandle_t_ *p = (IFFHandle_t_*)ctx;
    int32_t ret;    

    if (p->handle == 0) {
        p->last_ioerr = ERROR_OBJECT_NOT_FOUND;
        return -1;
    }
    if ((ret = p->flush(ctx)) < 0) {
        return ret;
    }

    Close(p->handle);
    p->handle = 0;
    p->last_ioerr = 0;
    
    return 0;
}

static int32_t s_write(IFFHandle_t *ctx, void *buffer, int32_t length)
{
    IFFHandle_t_ *p = (IFFHandle_t_*)ctx;
    int32_t ret = length;
    UBYTE *buf = buffer;

    if (p->handle == 0) {
        p->last_ioerr = ERROR_OBJECT_NOT_FOUND;
        return -1;
    }

    /* simple buffering to avoid too many direct writes into
     * the filesystem.. lesson learned from old StoneCracker
     * cruncher whose 'decrunch execuatble' & save was painfully
     * slow if there was any decent number of relocation entries.
     * All 4 bytes entries were written individually..
     */

    if (p->buffer_pos + length < p->buffer_size) { 
        memcpy(&p->buffer[p->buffer_pos],buf,length);
        p->buffer_pos += length;
    } else {
        /* n = how many bytes over the available buffer */
        int32_t n = p->buffer_pos + length - p->buffer_size;

        /* copy to the end of the available buffer */
        memcpy(&p->buffer[p->buffer_pos],buf,length-n);

        /* write/flush the entire buffer */
        ret = Write(p->handle,p->buffer,p->buffer_size);

        if (ret > 0) {
            if (n > 0) {
                /* copy the remaining of the user provided buffer */
                memcpy(p->buffer,&buf[length-n],n);
            }

            p->buffer_pos = n;
            ret = length;
        }
    }

    return ret;
}

static int32_t s_putc(IFFHandle_t *ctx, uint8_t ch)
{
    return ctx->write(ctx,&ch,1);
}

static int32_t s_putnum(IFFHandle_t *ctx, uint32_t num, int32_t length)
{
    IFFHandle_t_ *p = (IFFHandle_t_*)ctx;

    if (length < 1 || length > 4) {
        p->last_ioerr = ERROR_BAD_NUMBER;
        return -1;
    }

    num = num << ((4-length)*8);

    /* assume big endian.. this is important */
    return ctx->write(ctx,&num,length);
}

static int32_t s_seek(IFFHandle_t *ctx, int32_t position, int32_t mode)
{
    /* OFFSET_BEGINNING, OFFSET_CURRENT or OFFSET_END */
static int32_t s_putnum(IFFHandle_t *ctx, uint32_t, int32_t length);
    IFFHandle_t_ *p = (IFFHandle_t_*)ctx;

    if (p->handle == 0) {
        p->last_ioerr = ERROR_OBJECT_NOT_FOUND;
        return -1;
    }
    if (p->buffer_pos > 0) {
        /* before seek() alway flush the buffer.. makes life easier */
        int ret = p->flush(ctx);
        if (ret < 0) {
            return ret;
        }
    }

    return Seek(p->handle,position,mode);
}

static int32_t s_ioerr(IFFHandle_t *ctx)
{
    IFFHandle_t_ *p = (IFFHandle_t_*)ctx;

    /* check if we have a manually entered IoErr.. */
    if (p->last_ioerr == 0) {
        p->last_ioerr = IoErr();
    }
    return p->last_ioerr;
}

static int32_t s_flush(IFFHandle_t *ctx) {
    IFFHandle_t_ *p = (IFFHandle_t_*)ctx;
    int32_t ret;

    if (p->handle == 0) {
        p->last_ioerr = ERROR_OBJECT_NOT_FOUND;
        return -1;
    }
    if (p->buffer_pos > 0) {
        ret = Write(p->handle,p->buffer,p->buffer_pos);

        if (ret < 0) {
            return -1;
        }

        p->buffer_pos = 0;
    }

    return 0;
}
