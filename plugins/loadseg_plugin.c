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
#include <stdbool.h>
#include <exec/libraries.h>
#include <exec/types.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include "loadseg_plugin.h"
#include "utils.h"


struct ExecBase *SysBase   = NULL;
struct DosLibrary *DOSBase = NULL;

static void* local_init(__reg("a0") recv_cb r,
	__reg("a1") send_cb s,
	__reg("a2") dt_header_t *h,
	__reg("a3") void* p);
static LONG local_done(__reg("a0") void* ctx);
static LONG local_exec(__reg("a0") void* ctx);
static LONG local_run(__reg("a0") void* ctx);
static LONG local_errno(__reg("a0") void* ctx);

const struct loadseg_plugin plugin_info = {
	{
	LOADSEG_PLUGIN_ID,
	LOADSEG_PLUGIN_MAJOR,
	LOADSEG_PLUGIN_MINOR,
	LOADSEG_RESERVED,
	"$VER: loadseg_plugin "MSTR(LOADSEG_PLUGIN_MAJOR)"."MSTR(LOADSEG_PLUGIN_MINOR)" (23.9.2023) by Jouni 'Mr.Spiv' Korhonen",
	"Demotool Loadseg() plugin",

	local_init,
	local_exec,
	local_run,
	local_done,
	local_errno
	}
};

static void* local_init(__reg("a0") recv_cb r, __reg("a1") send_cb s, __reg("a2") dt_header_t *h, __reg("a3") void* p)
{
	context_t *p_ctx;
	SysBase = *((struct ExecBase **)4);

	/* DOSBase could be part of context as well.. */
	if (DOSBase = (struct DosLibrary*)OpenLibrary("dos.library",36)) {
		if (p_ctx = AllocVec(sizeof(context_t), 0)) {
			p_ctx->recv = r;
			p_ctx->send = s;
			p_ctx->p_hdr = h;
			p_ctx->user = p;
			p_ctx->program = 0;
			p_ctx->filename[0] = '\0';
			return p_ctx;
		}
	}

	return NULL;
}

static LONG local_done(__reg("a0") void* ctx)
{
	context_t *p_ctx = ctx;

	if (DOSBase) {
		if (p_ctx && p_ctx->program) {
			UnLoadSeg(p_ctx->program);
		}

		CloseLibrary((struct Library*)DOSBase);
		DOSBase = NULL;
	}
	if (p_ctx) {
		FreeVec(p_ctx);
	}

	return DT_ERR_OK;
}


#define TMP_BUF_LEN 1024
static s_tmp_buf[TMP_BUF_LEN];

static LONG local_exec(__reg("a0") void* ctx)
{
	context_t *p_ctx = ctx;
	uint32_t ret;
	int n;
	BPTR fh;
	LONG len, cnt;
	char a_prefix[15 + 1 +1];	/* max extension is 15 */
	bool override_dev = false;

	if (p_ctx->p_hdr->flags & DT_FLG_EXTENSION) {
		/* calculate the max size of the extension.. assume
		 * header version 0.
		 */
		int hdr_len = 4 + p_ctx->p_hdr->hdr_tag_len_ver[3] & DT_HDR_LEN_MASK;
		int ext_len = hdr_len - sizeof(dt_header_t);

		if ((n = find_extension(&p_ctx->p_hdr->extension[0],
			ext_len, DT_EXT_DEVICE_NAME, a_prefix)) >= 0) { 
			a_prefix[n] = '\0';
			override_dev = true;
		}
	}

	/* build temp filename.. default it being in RAM: */
	if (override_dev) {
		strcat(a_prefix,":");
	} else {
		strcpy(a_prefix,"RAM:");
	}
	if (get_tmp_filename(a_prefix,p_ctx->filename,LOADSEG_FILENAME_LEN) == NULL) {
		Printf("**Error: get_tmp_filename() failed\n");
		return DT_ERR_FILE_OPEN;
	}
	if ((fh = Open(p_ctx->filename,MODE_NEWFILE)) == 0) {
		ret = DT_ERR_FILE_OPEN;
		goto local_exec_exit;
	}

	cnt = 0;
	ret = DT_ERR_OK;

	/* read from socket and write to a file */
	while (cnt < p_ctx->p_hdr->size) {
		len = p_ctx->recv(s_tmp_buf, TMP_BUF_LEN,p_ctx->user);

		if (len < 0) {
			ret = DT_ERR_RECV;
			break;
		}
		if (len > 0) {
			if (Write(fh, s_tmp_buf, len) < 0) {
				ret = DT_ERR_FILE_IO;
				break;
			}
			cnt += len;
		} else {
			break;
		}
	}

	Close(fh);

	/* LoadSeg() the temp file */
	if (ret == DT_ERR_OK) {
		p_ctx->program = LoadSeg(p_ctx->filename);

		if (p_ctx->program == 0) {
			ret = DT_ERR_EXEC;
		}
	}

	/* delete the temp file */
	DeleteFile(p_ctx->filename);
local_exec_exit:
	p_ctx->send(&ret, 4, p_ctx->user);
	return ret;
}

static  LONG local_run(__reg("a0") void* ctx)
{
	context_t *p_ctx = ctx;
	typedef  void (*func)(void);
	func start = (func)BADDR(p_ctx->program);

	if (start) {
		__asm("\tmovem.l d2-d7/a2-a6,-(sp)\n");
		(*start)();
		__asm("\tmovem.l (sp)+,d2-d7/a2-a6\n");
	}

	return DT_ERR_OK;
}

static LONG local_errno(__reg("a0") void* ctx)
{
	if (DOSBase) {
		Printf("local_errno()\n");
	}
	return DT_ERR_OK;	
}

