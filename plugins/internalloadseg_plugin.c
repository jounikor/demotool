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

#include <string.h>
#include <exec/libraries.h>
#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "internalloadseg_plugin.h"

struct ExecBase *SysBase   = NULL;
struct DosLibrary *DOSBase = NULL;
ULONG g_errno = DT_ERR_OK;

__saveds static void* local_init(__reg("a0") recv_cb r,
	__reg("a1") send_cb s,
	__reg("a2") dt_header_t *h,
	__reg("a3") void* p);
__saveds static LONG local_done(__reg("a0") void* ctx);
__saveds static LONG local_exec(__reg("a0") void* ctx);
__saveds static LONG local_run(__reg("a0") void* ctx);
__saveds static ULONG local_errno(__reg("a0") void* ctx);

const struct internalloadseg_plugin plugin_info = {
	{
	INTERNALLOADSEG_PLUGIN_ID,
	INTERNALLOADSEG_PLUGIN_MAJOR,
	INTERNALLOADSEG_PLUGIN_MINOR,
	INTERNALLOADSEG_RESERVED,
	"$VER: loadseg_plugin "MSTR(INTERNALLOADSEG_PLUGIN_MAJOR)"."MSTR(INTERNALLOADSEG_PLUGIN_MINOR)" (2.8.2023) by Jouni 'Mr.Spiv' Korhonen",
	"Over the network InternalLoadseg() program launcher",

	local_init,
	local_exec,
	local_run,
	local_done,
	local_errno
	}
};

__saveds static void* local_init(__reg("a0") recv_cb r, __reg("a1") send_cb s, __reg("a2") dt_header_t *h, __reg("a3") void* p)
{
	SysBase = *((struct ExecBase **)4);

	if (DOSBase = (struct DosLibrary*)OpenLibrary("dos.library",36)) {
		Printf("local_init()\n");
	}

	return DOSBase;
}

__saveds static LONG local_done(__reg("a0") void* ctx)
{
	if (DOSBase) {
		Printf("local_done()\n");
		CloseLibrary((struct Library*)DOSBase);
	}

	return DT_ERR_OK;
}


__saveds static LONG local_exec(__reg("a0") void* ctx)
{
	if (DOSBase) {
		Printf("local_exec()\n");
	}
	return DT_ERR_OK;
}

__saveds static LONG local_run(__reg("a0") void* ctx)
{
	if (DOSBase) {
		Printf("local_run()\n");
	}
	return DT_ERR_OK;
}

__saveds static ULONG local_errno(__reg("a0") void* ctx)
{
	return g_errno;
}


