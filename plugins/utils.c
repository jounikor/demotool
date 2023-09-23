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

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <clib/alib_protos.h>
#include "utils.h"


/**
 * \brief This function parses extensions from dt_header_t.
 *
 * \param[out] p_val A ptr to destination buffer for the 
 *                   extension value. Must not be less than
 *                   15 octets in size.
 *
 * \param[in] p_ext  A ptr to extension blob.
 * \param ext_len    Length of the entire extensions blob.
 * \param type       Type of the extension to search for.
 *
 *
 * \Return
 *   -1 if no 'type' extension found
 *   0 or greater the extension was found and the return value
 *     is the payload size stored into the p_val buffer.
 */
int find_extension(uint8_t *p_ext, int ext_len,  uint8_t type, uint8_t *p_val)
{
	int n = 0;
	uint8_t b;

	while (n < ext_len) {
		b = p_ext[n++];

		if ((b & 0xf0) == type) {
			int l = b & 0x0f;
			int m = 0;

			while (m < l) {
				p_val[m++] = p_ext[n++];
			}

			return m;
		}
	}

	return -1;
}

/**
 *
 *
 *
 */

static char s_i2h[] = {'0','1','2','3','4','5','6','7','8',
		'9','a','b','c','d','e','f'};

static ULONG s_seed = 0;

char* get_tmp_filename(char* p_prefix, char* p_name, int name_len)
{
	char* p_next;
	int n;
	ULONG tmp;

	n = strlen(p_prefix);

	if (s_seed == 0) {
		/* a lame seed setting */
		s_seed = (ULONG)p_prefix;
	}
	if (n + 8 + 1 > name_len) {
		return NULL;
	}

	tmp = FastRand(s_seed);
	s_seed ^= tmp;

	/* strcpy() should return a ptr to \0 of the dst.. */
#if 0
	p_next = strcpy(p_name,p_prefix);
#else
	strcpy(p_name,p_prefix);
	p_next = &p_name[n];
#endif

	for (n = 0; n < 8; n++) {
		*p_next++ = s_i2h[tmp & 0xf];
		tmp >>= 4;
	}
	*p_next = '\0';
	return p_next;
}
