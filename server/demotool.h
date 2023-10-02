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
#ifndef _DEMOTOOL_H_INCLUDED
#define _DEMOTOOL_H_INCLUDED

#include "plugin_header.h"
#include "protocol.h"


#define BSDSOCKETLIBRARY_VERSION	4

#define TEMPLATE	"P=PLUGINS/K,PORT/N,A=ADDRESS/K,D=DEVICE/K,DEBUG/S,PRUNE/S,DELAY/N"

#define RDAT_PLUGINS	0
#define RDAT_PORT		1
#define RDAT_ADDRESS	2
#define RDAT_DEVICE		3
#define RDAT_DEBUG		4
#define RDAT_PRUNE		5
#define RDAT_DELAY		6
#define RDAT_ARRAY_SIZE		7

#define RDAT_DEFAULT_PORT	9999
#define RDAT_DEFAULT_ADDR	"0.0.0.0"
#define RDAT_DEFAULT_PATH	"plugins"
#define RDAT_DEFAULT_DEVICE	"df0"
#define RDAT_DEFAULT_DELAY	2

typedef struct dt_cfg {
	struct Library *socketbase;
	long active_socket;
	uint16_t port;
	uint16_t delay;
	char *addr;
	long backlog;
	char *device;
} dt_cfg_t;


#define CONFIG __cfg
#define CONFIG_PTR dt_cfg_t *CONFIG
#define CONFIG_PTR_GET ((dt_cfg_t *)CONFIG)
#define SocketBase CONFIG_PTR_GET->socketbase

#endif /* _DEMOTOOL_H_INCLUDED */
