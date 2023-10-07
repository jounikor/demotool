/*
 * v0.1 (c) 2023 Jouni 'Mr.Spiv' Korhonen
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

#include <exec/libraries.h>
#include <exec/types.h>
#include <exec/io.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dos.h>

#include <devices/trackdisk.h>
#include "adf_plugin.h"
#include "utils.h"

#include <string.h>

struct ExecBase *SysBase = NULL;
ULONG g_errno = DT_ERR_OK;

__saveds static void* local_init(__reg("a0") recv_cb r,
    __reg("a1") send_cb s,
    __reg("a2") dt_header_t *h,
    __reg("a3") void* p);
__saveds static LONG local_done(__reg("a0") void* ctx);
__saveds static LONG local_exec(__reg("a0") void* ctx);
__saveds static LONG local_run(__reg("a0") void* ctx);
__saveds static ULONG local_errno(__reg("a0") void* ctx);

const struct plugin_common plugin_info = {
    ADF_PLUGIN_ID,
    ADF_PLUGIN_MAJOR,
    ADF_PLUGIN_MINOR,
    ADF_RESERVED,
    "$VER: adf_plugin "MSTR(ADF_PLUGIN_MAJOR)"."MSTR(ADF_PLUGIN_MINOR)" (7.10.2023) by Jouni 'Mr.Spiv' Korhonen",
    "OTN ADF writer and program launcher",
    local_init,
    local_exec,
    local_run,
    local_done,
    local_errno
};

static char *s_drives[NUMUNITS] = {"df0", "df1", "df2", "df3"};

#define TRACK_LEN NUMSECS*TD_SECTOR

__saveds static void* local_init(__reg("a0") recv_cb r, __reg("a1") send_cb s, __reg("a2") dt_header_t *h, __reg("a3") void* p)
{
    context_t *p_ctx;
    int drv_num = 0;
    char drv_str[DT_EXT_MAX_LEN + 1];
    int ext_len;
    int n;

    SysBase = *((struct ExecBase **)4);

    /* Only standard ADF size or 1 track for bootblocks supported.. */
    if (!(h->size == (TRACK_LEN*160) || 
          h->size == TRACK_LEN)) {
        g_errno = DT_ERR_ADF_FILE_SIZE;
        return NULL;
    }
    if ((p_ctx = AllocVec(sizeof(context_t), 0)) == NULL) {
        g_errno = DT_ERR_MALLOC;
        return NULL;
    }
    if ((p_ctx->io_data = AllocVec(TRACK_LEN,0)) == NULL) {
        FreeVec(p_ctx);
        g_errno = DT_ERR_MALLOC;
        return NULL;
    }

    p_ctx->recv = r;
    p_ctx->send = s;
    p_ctx->p_hdr = h;
    p_ctx->user = p;
    p_ctx->p_port = NULL;
    p_ctx->p_ioreq = NULL;
    p_ctx->device_opened = FALSE;

    /* find device if given.. default to df0: */
    if (h->flags & DT_FLG_EXTENSION) {
        ext_len = get_extensions_len(p);

        if (ext_len > 0) {
            if ((n = find_extension(h->extension, ext_len,
                DT_EXT_DEVICE_NAME, drv_str)) > 0) {
                drv_str[n] = '\0';

                while (drv_num < NUMUNITS) {
                    if (!strcmp(s_drives[drv_num],drv_str)) {
                        break;
                    } else {
                        ++drv_num;
                    }
                }

                if (drv_num == NUMUNITS) {
                    g_errno = DT_ERR_DISK_DEVICE;
                    goto local_init_exit;
                }
            }
        }
    }

    /* attempt opening trackdisk device */
    if (p_ctx->p_port = CreateMsgPort()) {
        if (p_ctx->p_ioreq = (struct IOExtTD*)CreateIORequest(p_ctx->p_port,
                sizeof(struct IOExtTD))) {
            if (OpenDevice(TD_NAME, drv_num, (struct IORequest*)p_ctx->p_ioreq, 0) == 0) {
                p_ctx->device_opened = TRUE;
                return p_ctx;
            }

            g_errno = DT_ERR_OPENDEVICE;
            goto local_init_exit;
        }

        g_errno = DT_ERR_CREATE_IOREQ;
        goto local_init_exit;
    }

    g_errno = DT_ERR_CREATE_MSGPORT;
local_init_exit:
    local_done(p_ctx);
    return NULL;
}

__saveds static LONG local_done(__reg("a0") void* ctx)
{
    context_t *p_ctx = ctx;

    if (p_ctx) {
        if (p_ctx->io_data) {
            FreeVec(p_ctx->io_data);
        }
        if (p_ctx->device_opened) {    
            CloseDevice((struct IORequest*)p_ctx->p_ioreq);
        }
        if (p_ctx->p_ioreq) {
            DeleteIORequest((struct IORequest*)p_ctx->p_ioreq);
        }
        if (p_ctx->p_port) {
            DeleteMsgPort(p_ctx->p_port);
        }
        FreeVec(p_ctx);
    }
    return DT_ERR_OK;
}


__saveds static LONG local_exec(__reg("a0") void* ctx)
{
    context_t *p_ctx = ctx;
    ULONG cnt = 0;
    ULONG ret = DT_ERR_OK;

    /* read from socket and write to a file */
    while (cnt < p_ctx->p_hdr->size) {
        int len = 0;
        int pos = 0;

        do {
            if (TRACK_LEN - pos > 1024) {
                len = 1024;
            } else {
                len = TRACK_LEN - pos;
            }

            len = p_ctx->recv(p_ctx->io_data+pos, len, p_ctx->user);

            if (len <= 0) {
                break;
            } else {
                pos += len;
            }
        } while (pos < TRACK_LEN);

        if (len < 0) {
            ret = DT_ERR_RECV;
            break;
        }
        if (pos == TRACK_LEN) {
            /* Format a track with an ADF content */
            p_ctx->p_ioreq->iotd_Req.io_Command = TD_FORMAT;
            p_ctx->p_ioreq->iotd_Req.io_Length  = TRACK_LEN;
            p_ctx->p_ioreq->iotd_Req.io_Data    = (APTR)p_ctx->io_data;
            p_ctx->p_ioreq->iotd_Req.io_Offset  = cnt;
            p_ctx->p_ioreq->iotd_Req.io_Flags   = 0;
            cnt += TRACK_LEN;

            if (DoIO((struct IORequest*)p_ctx->p_ioreq) != 0) {
                ret = DT_ERR_DISK_IO | (UBYTE)p_ctx->p_ioreq->iotd_Req.io_Error;
                break;
            }
        } else {
            ret = DT_ERR_DISK_SECTOR_LEN;
            break;
        }
        if (len == 0) {
            break;
        }
    }

    /* turn motor off and neglect return value.. */
    p_ctx->p_ioreq->iotd_Req.io_Command = TD_MOTOR;
    p_ctx->p_ioreq->iotd_Req.io_Length  = 0;
    DoIO((struct IORequest*)p_ctx->p_ioreq);

    p_ctx->send(&ret, 4, p_ctx->user);
    return ret;
}

__saveds static LONG local_run(__reg("a0") void* ctx)
{
    return DT_ERR_OK;
}


__saveds static ULONG local_errno(__reg("a0") void* ctx)
{
    return g_errno;
}
