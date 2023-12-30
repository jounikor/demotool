/**
 * @file demotool.c
 * @brief A server side of the Amige remote code execution tool.
 * @author Jouni 'Mr.Spiv' Korhonen
 * @version 0.1
 * @date 2023
 * @note Programmed on A3000/040 and VBCC..
 * @licence
 * @copyright Unlicense
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

#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/rdargs.h>

/* defining __NOLIBBASE__ here after other protos have been included */
#define __NOLIBBASE__
#include <proto/bsdsocket.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <devices/trackdisk.h>

#include <string.h>
#include <stdint.h>

#include "demotool.h"
#include "plugin.h"

/* from plugins folder */
#include "plugin_header.h"
#include "utils.h"
/* from inc folder */
#include "protocol.h"

/*
 * For CLI Command Line parsing..
 */

LONG debug = 0;
static LONG rdat_array[RDAT_ARRAY_SIZE] = {0};

/* Disable fancy CTRL-C handling */
void _chkabort(void)
{
}

/*
 * Callback function implemenations. These are for
 * reading or writing data over the network.
 *
 * typedef LONG (*recv_cb)(__reg("a0") APTR, __reg("d0") LONG, __reg("a1") void*);
 * typedef LONG (*send_cb)(__reg("a0") APTR, __reg("d0") LONG, __reg("a1") void*);
 */

static LONG recv_callback(__reg("a0") APTR p_buf, __reg("d0") LONG len, __reg("a1") void* CONFIG)
{
    LONG tot = 0;
    LONG res;
    uint8_t *buf = p_buf;

    while (tot < len) {
        res = recv(CONFIG_GET_PTR->active_socket,buf+tot,len-tot,0);
    
        if (res < 0) {
            if (errno != EAGAIN) {
                tot = res;
                break;
            } 
        } else {
            tot += res;
        }
    }
    
    return tot;
}

static LONG send_callback(__reg("a0") APTR p_buf, __reg("d0") LONG len, __reg("a1") void* CONFIG)
{
    LONG tot = 0;
    LONG res;
    uint8_t *buf = p_buf;

    do {
        res = send(CONFIG_GET_PTR->active_socket,buf+tot,len-tot,0);

        if (res < 0) {
            if (errno != EAGAIN) {
                tot = res;
                break;
            }
        } else {
            tot += res;
        }
    } while (tot < len);

    return tot;
}

static uint32_t check_dt_ver_len(char* p_hdr, int* p_len, int* p_ver)
{
    int ver, len;

    if (strncmp(p_hdr,DT_HDR_TAG,3)) {
        return DT_ERR_HDR_TAG;
    }

    len = (int)(p_hdr[3] & DT_HDR_LEN_MASK);
    ver = (int)(p_hdr[3] & DT_HDR_VER_MASK);

    if (len == 0) {
        len = 256;
    }
    if (p_len) {
        *p_len = len;
    }
    if (p_ver) {
        *p_ver = ver;
    } 
    if (ver != DT_HDR_VERSION) {
        return -2;
    }
    if (len < sizeof(struct dt_header) - 4) {
        return DT_ERR_HDR_LEN;
    }

    return DT_ERR_OK;
}

static long init_with_open(CONFIG_PTR)
{
    long enable = 1;
    long sock;
    struct sockaddr_in sin;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        return -1;
    }
    if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&enable,sizeof(enable)) != 0) {
        CloseSocket(sock);
        return -1;
    }

    memset(&sin,0,sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = CONFIG_PTR_GET->port;
    sin.sin_addr.s_addr = inet_addr(CONFIG_PTR_GET->addr);

    if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) != 0) {
        CloseSocket(sock);
        return -1;
    }
    if (listen(sock, CONFIG_PTR_GET->backlog) != 0) {
        CloseSocket(sock);
        return -1;
    }

    return sock;
}

static int send_response(CONFIG_PTR, ULONG resp)
{
    return send(CONFIG_PTR_GET->active_socket, &htonl(resp), 4, 0);
}

static int recv_data(CONFIG_PTR, void *data, int len)
{
    return recv(CONFIG_PTR_GET->active_socket, data, len, 0);
}

static int close_connection(CONFIG_PTR)
{
    CloseSocket(CONFIG_PTR_GET->active_socket);
    CONFIG_PTR_GET->active_socket = -1;
    return DT_ERR_OK;
}

/**
 * @brief Wait for incoming connections.
 *
 * Wait for incoming connections inside a blcking accept().
 *
 * @param[in] CONFIG_PTR   A ptr to dt_cfg_t, which holds the current
 *                         runtime configuration for the server.
 * @param[in] listen_scok  A listening socket.
 * @return The "active_socket" for the incoming connection or -1
 *         in case of an error.
 */
static long accept_main(CONFIG_PTR, long listen_sock)
{
    long sock;
    struct sockaddr_in from;
    long from_len = sizeof(from);

    sock = accept(listen_sock,(struct sockaddr *)&from,&from_len);

    if (sock >= 0) {
        Printf("Connection from: %s:%lu\n",
            Inet_NtoA(from.sin_addr.s_addr),
            ntohs(from.sin_port));
    }

    return sock;
}

/**
 * @brief Main remote file loading and execution logic.
 *
 * This function does the main logic for loading and executing
 * the remote provided file.
 * The header gets parsed and proper plugin selected based on the
 * file from the remote client.
 *
 * @param[in] CONFIG_PTR   A ptr to dt_cfg_t, which holds the current
 *                         runtime configuration for the server.
 * @param[in] list         A ptr to a list of available and loaded
 *                         plugins.
 *
 * @return In case of success and most failures DT_CMD_PLUGIN is
 *         returned,which tells the caller to start waiting for
 *         the next incoming file.
 *         The return value can also be DT_CMD_QUIT or 
 *         DT_CMD_REBOOT based on the command sent by the remote
 *         client.
 */
static int loader_main(CONFIG_PTR, struct plugin_header* list)
{
    struct plugin_common* plugin;
    dt_header_t *p_hdr;
    int hdr_len, hdr_ver;
    char tag_len_ver[4];
    long ret;

    /* First read 4 first bytes for the type.. */
    ret = recv_data(CONFIG_PTR_GET, tag_len_ver, 4);

    if (ret != 4) {
        /* Error took place.. */
        Printf("**Error: failed to read header ID.\n");
        return DT_CMD_PLUGIN;
    }
    if (!strncmp(tag_len_ver,DT_HDR_TAG_CMD_QUIT,4)) {
        send_response(CONFIG_PTR_GET, DT_ERR_OK);
        return DT_CMD_QUIT;
    }
    if (!strncmp(tag_len_ver,DT_HDR_TAG_CMD_REBOOT,4)) {
        send_response(CONFIG_PTR_GET, DT_ERR_OK);
        return DT_CMD_REBOOT;
    }
    if ((ret = check_dt_ver_len(tag_len_ver, &hdr_len, &hdr_ver)) != DT_ERR_OK) {
        Printf("**Error: wrong container header.\n");
        send_response(CONFIG_PTR_GET, ret);
        return DT_CMD_PLUGIN;
    }
    if ((p_hdr = AllocVec(hdr_len + 4, 0)) == NULL) {
        send_response(CONFIG_PTR_GET, DT_ERR_MALLOC);
        return DT_CMD_PLUGIN;
    }
    
    memcpy(p_hdr->hdr_tag_len_ver, tag_len_ver, 4);
    ret = recv_data(CONFIG_PTR_GET, &p_hdr->plugin_tag_ver[0], hdr_len);

    if (ret != hdr_len) {
        /* Error took place.. */
        Printf("**Error: failed to read the header.\n");
        FreeVec(p_hdr);
        send_response(CONFIG_PTR_GET, DT_ERR_RECV);
        return DT_CMD_PLUGIN;
    }
    if (debug) {
        char tag1[4], tag2[5];
        int m, n = 0;

        tag1[0] = p_hdr->hdr_tag_len_ver[0];
        tag1[1] = p_hdr->hdr_tag_len_ver[1];
        tag1[2] = p_hdr->hdr_tag_len_ver[2];
        tag1[3] = '\0';

        tag2[0] = p_hdr->plugin_tag_ver[0]; 
        tag2[1] = p_hdr->plugin_tag_ver[1];
        tag2[2] = p_hdr->plugin_tag_ver[2];
        tag2[3] = p_hdr->plugin_tag_ver[3];
        tag2[4] = '\0';

        /* dump header */
        Printf("Requested plugin header and infos:\n");
        Printf("  Header tag: %s, length %ld, version %ld\n",
            tag1,hdr_len,hdr_ver);
        Printf("  Plugin tag: %s\n",tag2);
        Printf("  Plugin version: %ld.%ld\n", (ULONG)p_hdr->major, (ULONG)p_hdr->minor);
        Printf("  Plugin flags: $%08lx\n",p_hdr->flags);
        Printf("  Data size: %ld bytes\n",p_hdr->size);
        Printf("  Load/save address: $%08lx\n",p_hdr->addr);
        Printf("  Jump address: $%08lx\n",p_hdr->jump);

        if (p_hdr->flags & DT_FLG_EXTENSION) {
            Printf("Header has extensions:\n");
            while (n < (hdr_len - sizeof(dt_header_t) + 4)) {
                m = p_hdr->extension[n] & 0x0f;
                Printf("  Type %ld, len %ld: ",(ULONG)(p_hdr->extension[n++] & 0xf0),m);

                while (m-- > 0) {
                    Printf("%ld ",(ULONG)p_hdr->extension[n++]);
                }
                Printf("\n");
            }
        }
    }

    /* check for plugins.. */
    int exact_version = p_hdr->flags & DT_FLG_EXACT_VERSION ? 1 : 0;
    uint16_t version  = p_hdr->major << 8 | p_hdr->minor;
    plugin = find_plugin(list, &p_hdr->plugin_tag_ver[0], version, exact_version);

    if (plugin == NULL) {
        Printf("**Error: plugin not found.\n");
        FreeVec(p_hdr);
        send_response(CONFIG_PTR_GET, DT_ERR_NO_PLUGIN);
        return DT_CMD_PLUGIN;
    }

    /* plugin found.. do standard init stuff */
    if (debug) {
        Printf("Calling plugin init()\n");
    }
    void* ctx = plugin->init(recv_callback, send_callback, p_hdr, CONFIG_PTR_GET);

    if (ctx == NULL) {
        /* initializing the plugin failed.. */
        uint32_t init_error = plugin->errno(ctx);
        Printf("**Error: Initializing '%s' failed with %lu\n",
            plugin->description, init_error);
        plugin->done(NULL);
        FreeVec(p_hdr);
        send_response(CONFIG_PTR_GET, init_error);
        return DT_CMD_PLUGIN;
    }

    /* Header processed, plugin loaded and intialized succesfully.
     * ACK OK to the client and move to "plugin" phase.
     */
    send_response(CONFIG_PTR_GET, DT_ERR_OK);

    /* Plugin initialized.. execute plugin, which may mean loading
     * data over the network or sending something to the client.
     * The main program does not know or care.
     */
    if (debug) {
        Printf("Calling plugin exec()\n");
    }

    ret = plugin->exec(ctx);

    if (debug) {
        Printf("exec() returned %ld\n",ret);
    }

    /* Done executing.. since the main program owns the socket
     * close it before running teh actual code..
     */
    close_connection(CONFIG_PTR_GET);

    /* Skip running the plugin if execute failed.. In this case
     * it was plugin execution responsibility to send an error
     * back to the client..
     */
    if (!(p_hdr->flags & DT_FLG_NO_RUN) && ret == DT_ERR_OK) {
        if (debug) {
            Printf("Calling plugin run() with %lds delay\n",CONFIG_PTR_GET->delay);
        }
        if (CONFIG_PTR_GET->delay) {
            /* add some delay before running */
            Delay(50 * CONFIG_PTR_GET->delay);
        }
        ret = plugin->run(ctx);

        if (debug) {
            Printf("run() returned %ld\n",ret);
        }
    }

    /* about to exit.. we'll skip the return value since it has
     * no real use any more..
     */
    if (debug) {
        Printf("Calling plugin done()\n");
    }
    ret = plugin->done(ctx);

    if (debug) {
        Printf("done() returned %ld\n",ret);
    }
    if (ret == DT_ERR_OK && p_hdr->flags & DT_FLG_REBOOT_ON_EXIT) {
        ret = DT_CMD_REBOOT;
    } else {
        ret = DT_CMD_PLUGIN;
    }

    FreeVec(p_hdr);
    return ret;
}

/**
 * @brief The main server "idle loop" waiting for incoming connections.
 * 
 * This function initializes the server and waits for the incoming
 * connections from the remote clients. The server can be exited if
 * the remote client sent to "quit" command to the server.
 *
 * @param[in] CONFIG_PTR   A ptr to dt_cfg_t, which holds the current
 *                         runtime configuration for the server.
 * @param[in] list         A ptr to a list of available and loaded
 *                         plugins.
 *
 * @return DT_CND_QUIT or DT_CMD_ERROR + error code.
 */
static int start_server(CONFIG_PTR, struct plugin_header *list)
{
    long listen_sock;
    long backlog = 1;
    long sock;
    int ret;

    SocketBase = OpenLibrary("bsdsocket.library", BSDSOCKETLIBRARY_VERSION);

    if (SocketBase == NULL) {
        return DT_CMD_ERROR | DT_ERR_OPENLIBRARY;
    }
    if ((listen_sock = init_with_open(CONFIG_PTR_GET)) >= 0) {
        while ((sock = accept_main(CONFIG_PTR_GET,listen_sock)) >= 0) {
            CONFIG_PTR_GET->active_socket = sock;
            ret = loader_main(CONFIG_PTR_GET,list);

            if (CONFIG_PTR_GET->active_socket >= 0) {
                CloseSocket(CONFIG_PTR_GET->active_socket);
            }
            if (ret != DT_CMD_PLUGIN) {
                break;
            }
        }
    }
    if (listen_sock >= 0) {
        CloseSocket(listen_sock);
    }
    if (SocketBase != NULL) {
        CloseLibrary(SocketBase);
    }

    /* check for the actual exit code if it was a command */
    switch (ret) {
        case DT_CMD_REBOOT:
            Printf("Got remote command to reboot.\n");
            Delay(50);
            ColdReboot();
            break;
        case DT_CMD_QUIT:
            Printf("Got remote command to quit.\n");
        default:
            break;
    }

    return ret;
}


int main(void)
{
    int n;
    int num_plugins;
    struct dt_cfg cfg;
    char* plugin_path = RDAT_DEFAULT_PATH;
    LONG prune = 0;

    struct plugin_header* plugin_list = NULL;
    struct plugin_header** pruned_plugin_list = NULL;
    struct RDArgs *p_args;

    cfg.port = htons(RDAT_DEFAULT_PORT);
    cfg.delay = htons(RDAT_DEFAULT_DELAY);
    cfg.addr = RDAT_DEFAULT_ADDR;
    cfg.backlog = 1;
    cfg.device = RDAT_DEFAULT_DEVICE; 

    if ((p_args = ReadArgs(TEMPLATE,rdat_array,NULL)) == NULL) {
        Printf( "Usage:\n"
                "  demotool %s\n",TEMPLATE);
        Exit(20);
    }

    if (rdat_array[RDAT_PLUGINS]) {
        plugin_path = (char*)rdat_array[RDAT_PLUGINS];
    }
    if (rdat_array[RDAT_PORT]) {
        cfg.port =  htons(*(LONG *)rdat_array[RDAT_PORT]);
    }
    if (rdat_array[RDAT_DELAY]) {
        cfg.delay =  htons(*(LONG *)rdat_array[RDAT_DELAY]);
    }
    if (rdat_array[RDAT_ADDRESS]) {
        cfg.addr = (char*)rdat_array[RDAT_ADDRESS];
    }
    if (rdat_array[RDAT_DEVICE]) {
        cfg.device = (char*)rdat_array[RDAT_ADDRESS];
    }
    
    debug = rdat_array[RDAT_DEBUG];
    prune = rdat_array[RDAT_PRUNE];

    FreeArgs(p_args);

    /* Seacrh for plugins */

    if (debug) {
        Printf("Plugin path is '%s'.\n",plugin_path);
    }

    num_plugins = load_plugins(plugin_path,&plugin_list);

    if (num_plugins <= 0) {
        Printf("**Error: loading plugins failed or no plugins found.\n");
        Exit(20);
    }
    if (debug) {
        Printf("** Found %ld plugins:\n",num_plugins);
        struct plugin_header* hdr = plugin_list;

        for (n=0; n<num_plugins; n++) {
            Printf("Plugin #%ld ------------------>\n",n);
            dump_plugin(hdr);
            hdr = hdr->next;
        }
    }
    if (prune) {
        pruned_plugin_list = prune_plugin_list(plugin_list,&num_plugins);
        
        if (debug) {
            Printf("\n** %ld Plugins after pruning:\n",num_plugins);
            n = 0;

            while (n < num_plugins) {
                Printf("Plugin #%ld ------------------>\n",n);
                dump_plugin(pruned_plugin_list[n++]);
            }
        }
    }

    struct plugin_header *plugins;
    plugins = *pruned_plugin_list ? *pruned_plugin_list : plugin_list;

    if (start_server(&cfg, plugins) & DT_CMD_ERROR) {
        Printf("**Error: starting server failed.\n");
    }
    if (pruned_plugin_list) {
        FreeVec(pruned_plugin_list);
    }

    release_plugins(plugin_list);
    return 0;
}
