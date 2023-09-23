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
#include <stdint.h>
#include "protocol.h"

#define PLUGIN_MAGIC	0x70ff4e75	/* moveq #1,d0; rts */
#define PLUGIN_TAGVER	0x504c4730	/* "PLG0" */
#define PLUGIN_TAGMASK	0xffffff00
#define PLUGIN_VERMASK	0x000000ff

/* Use MSTR() for $VER string to expand version numbers */
#define TSTR(s) #s
#define MSTR(s) TSTR(s)

/*
 *
 *
 */
struct plugin_header {
	uint32_t magic;					/* moveq #-1,d0; ret; */
	uint32_t tag_ver;				/* "PLG0" tag + header version */
	BPTR seglist;					/* loadseg() of the plugin file */
	struct plugin_common *info;		/* common part */
	struct plugin_header *next;		/* for chaining plugins */
};

/*
 * Common part of every plugin structure..
 */

typedef LONG (*recv_cb)(__reg("a0") APTR, __reg("d0") LONG, __reg("a1") void*);
typedef LONG (*send_cb)(__reg("a0") APTR, __reg("d0") LONG, __reg("a1") void*);

struct plugin_common {
	uint32_t plugin_id;				/* to identify the plugin */
	uint8_t major;					/* version high */
	uint8_t minor;					/* version low */
	uint16_t reserved;
	const char *version_str;		/* standard version string */
	const char *description;		/* human readable plugin description */

	/* fixed plugin functions..
	 *
	 * Paramas:
	 *  r - 
	 *  s -
	 *  h -
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
	LONG  (*errno)(__reg("a0") void* ctx);
};

#define INVALID_PLUGIN_ID 0



#endif 	/* _PLUGIN_HEADER_H_INCLUDED */

