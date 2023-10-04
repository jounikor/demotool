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
    "$VER: adf_plugin "MSTR(ADF_PLUGIN_MAJOR)"."MSTR(ADF_PLUGIN_MINOR)" (2.8.2023) by Jouni 'Mr.Spiv' Korhonen",
    "OTN ADF writer and program launcher",
    local_init,
    local_exec,
    local_run,
    local_done,
    local_errno
};

static char *s_drives[NUMUNITS] = {"df0", "df1", "df2", "df3"};

#define TMP_BUF_LEN NUMSECS*TD_SECTOR

__saveds static void* local_init(__reg("a0") recv_cb r, __reg("a1") send_cb s, __reg("a2") dt_header_t *h, __reg("a3") void* p)
{
    context_t *p_ctx;
    int drv_num = 0;
    char drv_str[DT_EXT_MAX_LEN + 1];
    int ext_len;
    int n;

    SysBase = *((struct ExecBase **)4);

    if ((p_ctx = AllocVec(sizeof(context_t) + TMP_BUF_LEN, 0)) == NULL) {
        g_errno = DT_ERR_MALLOC;
        return NULL;
    }

    p_ctx->io_data = ((uint8_t*)p_ctx) + sizeof(context_t);
    p_ctx->recv = r;
    p_ctx->send = s;
    p_ctx->p_hdr = h;
    p_ctx->user = p;
    p_ctx->p_port = NULL;
    p_ctx->p_ioreq = NULL;
    p_ctx->device_opened = false;

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
        if (p_ctx->p_ioreq = (struct IOStdReq*)CreateIORequest(p_ctx->p_port,
                sizeof(struct IOStdReq))) {
            if (OpenDevice(TD_NAME, drv_num, (struct IORequest*)p_ctx->p_ioreq, 0) == 0) {
                p_ctx->device_opened = true;
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
        if (p_ctx->device_opened) {    
            CloseDevice((struct IORequest*)p_ctx->p_ioreq);
        }
        if (p_ctx->p_ioreq) {
            DeleteIORequest(p_ctx->p_ioreq);
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
    LONG len;
    uint32_t ret = DT_ERR_OK;

    /* read from socket and write to a file */
    while (cnt < p_ctx->p_hdr->size) {
        len = p_ctx->recv(p_ctx->io_data, TMP_BUF_LEN, p_ctx->user);

        if (len < 0) {
            ret = DT_ERR_RECV;
            break;
        }
        if (len > 0) {
            p_ctx->p_ioreq->io_Command = CMD_WRITE;
            p_ctx->p_ioreq->io_Length  = (ULONG)len;
            p_ctx->p_ioreq->io_Data    = (APTR)p_ctx->io_data;
            p_ctx->p_ioreq->io_Offset  = cnt;

            if (DoIO((struct IORequest*)p_ctx->p_ioreq) != 0) {
                ret = DT_ERR_DISK_IO | p_ctx->p_ioreq->io_Error;
                break;
            }
            cnt += len;
        } else {
            break;
        }
    }

    /* tuen motor off and neglect return value.. */
    p_ctx->p_ioreq->io_Command = TD_MOTOR;
    p_ctx->p_ioreq->io_Length  = 0;
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


