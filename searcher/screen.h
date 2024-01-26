/*
 * v0.1 (c) 2024 Jouni 'Mr.Spiv' Korhonen
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

#ifndef _SCREEN_H_INCLUDED
#define _SCREEN_H_INCLUDED

#include <intuition/intuition.h>
#include <exec/types.h>

/* Function prototypes.. unneeded but still here */
BOOL setup_sprites(struct Screen *p_screen);
struct UCopList* setup_copper(struct Screen* p_scr, cop_cfg_t* p_cfg);
void update_head_texts(struct Window *p_window, BOOL statics, ULONG texts, cop_cfg_t *p_cfg);
void update_pal_texts(struct Window *p_window, BOOL on);
ULONG init_cop_cfg(cop_cfg_t *p_cfg, BOOL pal_only);


#endif  /*  _SCREEN_H_INCLUDED */
