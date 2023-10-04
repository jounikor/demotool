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
#ifndef _PLUGIN_HEADER_H_INCLUDED
#define _PLUGIN_HEADER_H_INCLUDED

#include <dos/dos.h>
#include <exec/types.h>
#include "protocol.h"

#define PLUGIN_MAGIC    0x70ff4e75  /* moveq #1,d0; rts */
#define PLUGIN_TAGVER   0x504c4730  /* "PLG0" */
#define PLUGIN_TAGMASK  0xffffff00
#define PLUGIN_VERMASK  0x000000ff

/* Use MSTR() for $VER string to expand version numbers */
#define TSTR(s) #s
#define MSTR(s) TSTR(s)

/*
 *
 *
 */
struct plugin_header {
    ULONG magic;                /* moveq #-1,d0; ret; */
    ULONG tag_ver;              /* "PLG0" tag + header version */
    BPTR  seglist;              /* loadseg() of the plugin file */
    struct plugin_common *info; /* common part */
    struct plugin_header *next; /* for chaining plugins */
};

/*
 * Common part of every plugin structure..
 *
 * Note that the plugin_header has a pointer to the 
 * plugin specific plugin_common structure. However,
 * the plugin specific structure does not need to be
 * exactly the plugin_common. It is perfectuly fine to
 * use any structure that just begings with the plugin_common.
 * Example:
 * 
 *  struct foo_plugin {
 *    struct plugin_common common;
 *    LONG foo_long;
 *    ...
 *  };
 *
 * The plugin_header.c does a typecast in any case to 
 * 'struct plugin_header *'. As long as 'struct foo_plugin'
 * is named as plugin_info.
 *
 * Each plugin must implement all functions the struct
 * plugin_common introduces. Note that both done() and
 * errno() functions must be able to work even if the
 * provided context pointer is NULL.
 *
 * The plugin exported functions are called by the server:
 *
 *  init() - always.
 *  exec() - if init() succeeded.
 *  run()  - if DT_FLD_NO_RUN is not set and init() exec()
 *           succeeded.
 *  done() - if init() succeeded.
 */

typedef LONG (*recv_cb)(__reg("a0") APTR, __reg("d0") LONG, __reg("a1") void*);
typedef LONG (*send_cb)(__reg("a0") APTR, __reg("d0") LONG, __reg("a1") void*);

struct plugin_common {
    ULONG plugin_id;            /* to identify the plugin */
    UBYTE major;                /* version high */
    UBYTE minor;                /* version low */
    USHORT reserved;
    const STRPTR version_str;   /* standard version string */
    const STRPTR description;   /* human readable plugin description */

    /* fixed plugin functions..
     *
     * Paramas:
     *  r - a ptr to recv() callback function. 
     *  s - a ptr to send() callback function.
     *  h - a ptr to dt_header_t received from the client.
     *  p - A ptr to user data. This will be passed to callback
     *      functions as the user context data.
     *
     * Return:
     *  NULL if error. Otherwise a ptr to plugin private
     *  context data structure that must be passed to other
     *  plugin methods.
     */
    void* (*init)(__reg("a0") recv_cb r,
            __reg("a1") send_cb s,
            __reg("a2") dt_header_t *h,
            __reg("a3") void* p);
    LONG  (*exec)(__reg("a0") void* ctx);
    /* Typically run() returns what the executed program returned */
    LONG  (*run)(__reg("a0") void* ctx);
    /* Must be able to handle case 'ctx == NULL' 
     * After calling done() the 'ctx' becomes invalid.
     */
    LONG  (*done)(__reg("a0") void* ctx);
    /* must be able to handle case 'ctx == NULL' */
    ULONG  (*errno)(__reg("a0") void* ctx);
};

#define INVALID_PLUGIN_ID 0

#endif  /* _PLUGIN_HEADER_H_INCLUDED */
