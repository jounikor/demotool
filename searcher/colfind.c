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

#include <intuition/intuition.h>
#include "colfind.h"

/*
 * Copper move instructions to color registers are
 * from 0x0180 to 0x01be. To check if any color register
 * match check for bits that must be 0:
 *  mask: 0b11111110 01000001
 *                 | | |||||
 * 0x0182   00000001 10000010
 * 0x0192   00000001 10010010
 * 0x01a2   00000001 10100010
 * 0x01b2   00000001 10110010
 *
 * Then the actual color RGB value mask:
 *    0b1111000000000000
 *
 */

ULONG copper_search(cop_cfg_t *p_cfg, int qual, int min_num)
{
    LONG  end_pos, found_pos, current_pos;
    int found = 0;
    UWORD *mem = (UWORD*)0x000000;
    LONG move_mask = 0xfe41;
    LONG reg_mask  = 0x0180;
    LONG rgb_mask  = 0xf000;
    LONG chip_mask = (p_cfg->max_chipmem - 1) >> 1;
    BOOL move = TRUE;
    UWORD reg, idx, rgb;
    ULONG colpal = 0;
    LONG step = 1;

    current_pos = p_cfg->pal_ptr >> 1;

    if (qual & IEQUALIFIER_LSHIFT) {
        step = 32;
    }
    if (current_pos > 0) {
        current_pos = (current_pos + step) & chip_mask;
    }

    end_pos = current_pos;
    found_pos = current_pos;
    p_cfg->ioerr = 0;

    do {
        if (move) {
            reg = mem[current_pos];
            idx = (reg & 0x3e) >> 1;

            if (((reg & move_mask) == 0) && 
                ((reg & reg_mask) == reg_mask) &&
                !(colpal & (1 << idx))) {

                if (found == 0) {
                    found_pos = current_pos;
                }

                move = FALSE;
            } else {
                found = 0;
                colpal = 0;
            }
        } else {
            rgb = mem[current_pos];

            if (((rgb & rgb_mask) == 0)) {
                ++found;
                colpal |= (1 << idx);
            } else {
                found = 0;
                colpal = 0;
            }
            move = TRUE;
        }

        current_pos = (current_pos + 1) & chip_mask;
        *((UWORD*)0xdff184) = current_pos;
    } while (current_pos != end_pos && found < min_num);

    if (found == min_num) {
        p_cfg->pal_ptr = found_pos << 1;

        for (int n = 0; n < 32; n++) {
            p_cfg->pal[n] = 0;
        }

        for (int n = 0; n < 32; n++) {
            reg = mem[found_pos++];
            rgb = mem[found_pos++];
            idx = (reg & 0x3e) >> 1;

            if (((reg & move_mask) == 0) && 
                ((reg & reg_mask) == reg_mask) &&
                ((rgb & rgb_mask) == 0)) {

                p_cfg->pal[idx] = rgb;
            }

            found_pos &= chip_mask;
        }

        p_cfg->status = IDX_STATUS_PALETTE;
        return TEXT_ADDRESS|RGB_CHANGE|TEXT_STATUS;
    } else {
        p_cfg->status = IDX_STATUS_NOTFOUND;
        return TEXT_STATUS;
    }
}
