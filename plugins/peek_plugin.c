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
#include <proto/dos.h>
#include <proto/exec.h>
#include "peek_plugin.h"

struct ExecBase *SysBase   = NULL;
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
    PEEK_PLUGIN_ID,
    PEEK_PLUGIN_MAJOR,
    PEEK_PLUGIN_MINOR,
    PEEK_RESERVED,
    "$VER: peek_plugin "MSTR(PEEK_PLUGIN_MAJOR)"."MSTR(PEEK_PLUGIN_MINOR)" (28.9.2023) by Jouni 'Mr.Spiv' Korhonen",
    "OTN memory viever plugin",

    local_init,
    local_exec,
    local_run,
    local_done,
    local_errno
};


__saveds static void* local_init(__reg("a0") recv_cb r, __reg("a1") send_cb s, __reg("a2") dt_header_t *h, __reg("a3") void* p)
{
    SysBase = *((struct ExecBase **)4);
    context_t *p_ctx;

    /* sanity checks */
    if (h->size == 0) {
        g_errno = DT_ERR_HDR_INVALID;
        return NULL;
    }
    /* First some sanity tests */
    if (p_ctx = AllocVec(sizeof(context_t), 0)) {
        p_ctx->recv = r;
        p_ctx->send = s;
        p_ctx->p_hdr = h;
        p_ctx->user = p;
        return p_ctx;
    }        

    g_errno = DT_ERR_MALLOC;
    return NULL;
}

__saveds static LONG local_done(__reg("a0") void* ctx)
{
    context_t *p_ctx = ctx;

    if (p_ctx) {
        FreeVec(p_ctx);
    }

    return DT_ERR_OK;
}

__saveds static LONG local_run(__reg("a0") void* ctx)
{
    return DT_ERR_OK;
}

__saveds static ULONG local_errno(__reg("a0") void* ctx)
{
    return g_errno;
}

#define TMP_BUF_LEN 1024

__saveds static LONG local_exec(__reg("a0") void* ctx)
{
    context_t *p_ctx = ctx;
    dt_header_t *p_hdr = p_ctx->p_hdr;
    uint8_t *p_dst;
    LONG len;
    uint32_t size, cnt;
    uint32_t ret = DT_ERR_OK;

    p_dst = (uint8_t*)p_hdr->addr;
    size = p_hdr->size;
    cnt = 0;

    /* read from socket and write to a file */
    while (cnt < size) {
        len = size - cnt >= TMP_BUF_LEN ? TMP_BUF_LEN : size - cnt;
        len = p_ctx->send(p_dst, len, p_ctx->user);

        if (len < 0) {
            g_errno = DT_ERR_SEND;
            ret = DT_ERR_SEND;
            break;
        }
        cnt += len;
        p_dst += len;
    }
    return ret;
}
