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

/* Amiga specific includes */
#include <exec/types.h>
#include <dos/dos.h>

#include <proto/dos.h>
#include <proto/exec.h>

/* stdlib, libamiga includes */
#include <string.h>
#include <stdlib.h>
#include <assert.h>

/* from plugin folder */
#include "plugin_header.h"

extern LONG debug;

static int comp(const void* a, const void* b)
{
    struct plugin_header *p_a = *(struct plugin_header**)a;
    struct plugin_header *p_b = *(struct plugin_header**)b;

    if (p_a->info->plugin_id < p_b->info->plugin_id) {
        return -1;
    }
    if (p_a->info->plugin_id > p_b->info->plugin_id) {
        return 1;
    }

    /* plugin ids are equal.. check verions */
    uint16_t v_a = p_a->info->major << 8 | p_a->info->minor;
    uint16_t v_b = p_b->info->major << 8 | p_b->info->minor;

    if (v_a < v_b) {
        return 1;
    }
    if (v_a > v_b) {
        return -1;
    }

    /* this should not happen.. more than one plugin with different name
     * but same version
     */
     
    return 0;

}

struct plugin_header** prune_plugin_list(struct plugin_header* head, int* size)
{
    struct plugin_header** table;
    struct plugin_header* list = head;
    int num_entries = 0;
    int pos, n;
    uint32_t tag;

    if ((table = AllocVec((*size) * sizeof(struct plugin_header*),0)) == NULL) {
        return NULL;
    }

    /* Collect list and sort it */

    list = head;
    num_entries = 0;

    while (list) {
        table[num_entries++] = list;
        list = list->next;
    }

    qsort(table,num_entries,sizeof(struct plugin_header*),comp);

    /* next pickup only one of each plugin and the highest version */
    n = 0;
    pos = 0;
    tag = INVALID_PLUGIN_ID;

    while (n < num_entries) {
        list = table[n++];

        if (tag != list->info->plugin_id) {
            /* we advanced to the next plugin.. get the first */
            table[pos++] = list;
            tag = list->info->plugin_id;
        }
    }

    /* end mark */
    *size = pos;
    return table;
}


struct plugin_common* find_plugin(struct plugin_header* list,
    char* tag, uint16_t ver, int exact)
{
    while (list) {
        char* tagver_str = (char*)&list->info->plugin_id;
        uint16_t version = list->info->major << 8 |
            list->info->minor;

        if (!strncmp(tagver_str,tag,4)) {
            if (version >= ver && !exact) {
                return list->info;
            }
            if (version == ver && exact) {
                return list->info;
            }
        }

        list = list->next;
    }

    return NULL;
}

void dump_plugin(struct plugin_header *hdr)
{
    struct plugin_common* info;
    info = hdr->info;
    Printf("  -> Version: %s\n",info->version_str);
    Printf("  -> Description: %s\n",info->description);
}

int release_plugins(struct plugin_header *list)
{
    struct plugin_header *next;
    struct loadseg_plugin *lseg;
    struct adf_plugin *adf;
    int released = 0;

    while (list) {
        ++released;
        next = list->next;
        UnLoadSeg(list->seglist);
        list = next;
    }
    return released;
}

extern LONG rdat_array[];

int load_plugins(char *path, struct plugin_header **head)
{
    struct plugin_header *list = NULL;
    struct plugin_header *hdr;
    struct FileInfoBlock *fib;
    BPTR lock, olddir;
    BPTR seglist;
    APTR address;
    int num_found = 0;

    if ((fib = AllocDosObject(DOS_FIB,0)) != NULL) {
        if (lock = Lock(path,SHARED_LOCK)) {
            if (Examine(lock,fib)) {

                /* is the given path a directory? */
                if (fib->fib_DirEntryType > 0) {
                    olddir = CurrentDir(lock);

                    while (ExNext(lock,fib)) {
                        /* is this fib for a file? we have no recursion.. */
                        if (fib->fib_DirEntryType < 0) {
                            if (seglist = LoadSeg(fib->fib_FileName)) {
                                address = BADDR(seglist+1);
                                hdr = (struct plugin_header *)address;

                                if (hdr->magic == PLUGIN_MAGIC && hdr->tag_ver == PLUGIN_TAGVER) {
                                    hdr->seglist = seglist;
                                    ++num_found;

                                    if (list == NULL) {
                                        list = hdr;
                                        *head = list;
                                    } else {
                                        list->next = hdr;
                                        list = hdr;
                                    }
                                } else {
                                    UnLoadSeg(seglist);
                                }
                            }
                        }
                    }

                    if (IoErr() != ERROR_NO_MORE_ENTRIES) {
                        num_found = -num_found;
                    }

                    CurrentDir(olddir);
                }
            }
            UnLock(lock);
        }
        FreeDosObject(DOS_FIB,fib);
    }

    return num_found;
}

