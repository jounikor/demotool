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
#ifndef _INTERNALLOADSEG_PLUGIN_H_INCLUDED
#define _INTERNALLOADSEG_PLUGIN_H_INCLUDED

#include "plugin_header.h"


#define INTERNALLOADSEG_PLUGIN_MAJOR	0
#define INTERNALLOADSEG_PLUGIN_MINOR	1
#define INTERNALLOADSEG_PLUGIN_ID		0x6C736731
#define INTERNALLOADSEG_RESERVED 		0

struct internalloadseg_plugin {
	struct plugin_common common;
};

typedef struct context {
	recv_cb recv;
	send_cb send;
	dt_header_t *p_hdr;
	void *user;
} context_t;


#endif 	/* _INTERNALLOADSEG_PLUGIN_H_INCLUDED */

