/*
 * (c) 2023 by Jouni 'Mr.Spiv' Korhonen
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef __AMIGA__
#include <unistd.h>
#include <sys/select.h>
#include <fcntl.h>
#include <getopt.h>
#include <errno.h>
#else
#include <proto/exec.h>
#include <proto/bsdsocket.h>
#include <sys/ioctl.h>
#include "getopt.h"
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/ip.h> 

#include "protocol.h"
#include "demotool.h"

#ifdef __AMIGA__
/* Disable fancy CTRL-C handling */
void _chkabort(void)
{
}
#endif

static SOCK_T open_connection(char *name, uint16_t port)
{
    struct sockaddr_in sin;
    struct hostent* he;
    SOCK_T sock;
    struct timeval tv;
    int ret, opt, val;
    fd_set wait_set;
    socklen_t val_len = sizeof(val);

    if ((he = gethostbyname(name)) == NULL) {
        printf("**Error: name resolution for '%s' failed.\n",name);
        return DT_ERR_CLIENT | DT_ERR_DNS;
    }
    if ((sock = socket(AF_INET,SOCK_STREAM,0)) < 0) {
        printf("**Error: opening socket failed.\n");
        return DT_ERR_CLIENT | DT_ERR_SOCKET_OPEN;
    }
#ifndef __AMIGA__
	if ((opt = fcntl (sock, F_GETFL, NULL)) < 0) {
		return DT_ERR_CLIENT | DT_ERR_FCNTL;
	}
	if (fcntl (sock, F_SETFL, opt | O_NONBLOCK) < 0) {
#else
	opt = 1;
	if (IoctlSocket(sock, FIONBIO, &opt) != 0) {
#endif
		return DT_ERR_CLIENT | DT_ERR_FCNTL;
	}

    memset(&sin,0,sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = *(in_addr_t*)he->h_addr;

    if ((ret = connect(sock,(struct sockaddr*)&sin,sizeof(sin))) < 0) {
        if (errno == EINPROGRESS) {
            tv.tv_sec = DT_DEF_CONNECT_TIMEOUT;
            tv.tv_usec = 0;
            FD_ZERO (&wait_set);
			FD_SET (sock, &wait_set);
#ifndef __AMIGA__
            ret = select (sock + 1, NULL, &wait_set, NULL, &tv);
#else
			ret = WaitSelect(sock + 1, NULL, &wait_set, NULL, &tv, NULL);
#endif
        } else {
            printf("**Error: connect() failed %d\n",errno);
            sock = DT_ERR_CLIENT | DT_ERR_CONNECT;
            goto open_connection_exit;
        }
    } else {
        ret = 1;
    }
    if (ret == 0) {
        sock = DT_ERR_CLIENT | DT_ERR_CONNECT_TIMEOUT;
        goto open_connection_exit;
    }
    if (ret < 0) {
        sock = DT_ERR_CLIENT | DT_ERR_SELECT;
        goto open_connection_exit;
    }
    if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &val, &val_len) < 0) {
        sock = DT_ERR_CLIENT | DT_ERR_GETSOCKOPT;
        goto open_connection_exit;
    }
    if (val != 0) {
        printf("**Error: connect() failed#2 %d\n",val);
        sock = DT_ERR_CLIENT | DT_ERR_CONNECT_OTHER;
        goto open_connection_exit;
    }
#ifndef __AMIGA__
    if (fcntl(sock, F_SETFL, opt) < 0) {
#else
	opt = 0;
	if (IoctlSocket(sock, FIONBIO, &opt) != 0) {
#endif
        sock = DT_ERR_CLIENT | DT_ERR_FCNTL;
        goto open_connection_exit;
    }
    return sock;

open_connection_exit:
#ifndef __AMIGA__
    close(sock);
#else
	CloseSocket(sock);
#endif
    return sock;
}

static uint32_t handshake(SOCK_T s, dt_header_t* hdr, int hdr_len)
{
    int n;
    uint8_t resp[4];
    uint32_t ret;

    /* send header and check whether this is a plugin or a command */
    n = send(s,hdr,hdr_len,0);

    if (n != (hdr_len)) {
        printf("**Error: failed to send dt_header\n");
        return DT_ERR_CLIENT | DT_ERR_SEND;
    }

    /* read response */
    n = recv(s,resp,4,0);

    if (n != 4) {
        printf("**Error: failed to recv response.\n");
        return DT_ERR_CLIENT | DT_ERR_RECV;
    }

    ret  = resp[0] << 24;
    ret |= resp[1] << 16;
    ret |= resp[2] << 8;
    ret |= resp[3];
    return ret;
}

static int add_extension(uint8_t type, uint8_t* hdr, char* ext, int len)
{
    int pos = 0;

    /* truncate too big length values.. there is 4 bits for the length */
    if (len > 15) {
        len = 15;
    }

    hdr[pos++] = type | len;

    while (len-- > 0) {
        if (ext) {
            hdr[pos++] = *ext++;
        } else {
            hdr[pos++] = 0;
        }
    }

    return pos;
}


static dt_header_t* prepare_header(dt_options_t* p_opt, int* final_len)
{
    char ver_len;
    dt_header_t* p_hdr;
    int header_len = sizeof(dt_header_t);
    int extension_len = 0;
    int n;

    /* check for extensions */
    if (p_opt->flags & DT_FLG_EXTENSION) {
        if (p_opt->device) {
            /* Currently we only understand device name */
            extension_len += strlen(p_opt->device);
        }
    }
    if (header_len + extension_len - 4 > 256) {
        printf("**Error: header size too big.\n");
        return NULL;
    }
    if ((p_hdr = malloc(header_len + extension_len + 4)) == NULL) {
        printf("**Error: malloc() failed.");
        return NULL;
    }

    /* Version of this header.. */
    ver_len  = (header_len - 4 + extension_len + 3) & DT_HDR_LEN_MASK;
    ver_len |= DT_HDR_VERSION;

    /* Fill in header statics */
    p_hdr->hdr_tag_len_ver[0] = 'H';
    p_hdr->hdr_tag_len_ver[1] = 'D';
    p_hdr->hdr_tag_len_ver[2] = 'R';
    p_hdr->hdr_tag_len_ver[3] = ver_len;
    
    /* Requested plugin information */
    strncpy(&p_hdr->plugin_tag_ver[0],p_opt->plugin,4);
    p_hdr->major = p_opt->major & 0xff;
    p_hdr->minor = p_opt->minor & 0xff;
	p_hdr->reserved = 0;
    p_hdr->flags = htonl(p_opt->flags);
    p_hdr->addr  = htonl(p_opt->addr);
    p_hdr->jump  = htonl(p_opt->jump);
    p_hdr->size  = htonl(p_opt->size);

    /* Add extensions */
    n = 0;

    if (p_opt->flags & DT_FLG_EXTENSION) {
        if (p_opt->device) {
            n += add_extension(DT_EXT_DEVICE_NAME, &p_hdr->extension[n],
                p_opt->device, strlen(p_opt->device));
        }
    
        /* round up to next 4 bytes */
        while (n & 3) {
            /* add 0 length padding bytes.. could use length too */
            n += add_extension(DT_EXT_PADDING, &p_hdr->extension[n], NULL, 0);
        }
    }
    
    header_len += n;
    *final_len = header_len;
    return p_hdr;
}

static uint32_t get_file_read_size(char* name, FILE** pp_fh, bool close_on_return)
{
    FILE *fh;
    int offset;

    if ((fh = fopen(name,"rb")) == NULL) {
        return DT_ERR_CLIENT | DT_ERR_FILE_OPEN;
    }

    fseek(fh,0,SEEK_END);
    offset = ftell(fh);
    fseek(fh,0,SEEK_SET);

    if (offset < 0) {
        fclose(fh);
        return DT_ERR_CLIENT | DT_ERR_FILE_IO;
    }
    if (close_on_return) {
        fclose(fh);
    } else {
        *pp_fh = fh;
    }

    return (uint32_t)offset;
}

static uint32_t open_file_write(char* name, FILE** fh)
{
    return DT_ERR_CLIENT | DT_ERR_NOT_IMPLEMENTED;
}


#define TMP_BUF_LEN 1023
static uint8_t s_tmp_buf[TMP_BUF_LEN];

static uint32_t lsg0(SOCK_T s, dt_options_t* p_opts,  char* name)
{
    uint32_t ret;
    uint32_t size;
    int hdr_len;
	int len, cnt;
    FILE *fh;
    dt_header_t* hdr = NULL;

    size = get_file_read_size(name, &fh, false);
    p_opts->size = size;

    if (size & DT_ERR_CLIENT) {
        return size;
    }
    if ((hdr = prepare_header(p_opts,&hdr_len)) == NULL) {
        return DT_ERR_MALLOC;
    }
    if ((ret = handshake(s, hdr, hdr_len)) & DT_ERR_CLIENT) {
        printf("**Error: handshake failed %lu\n",ret);
        goto lsg0_exit;
    }

    /* read file in and send it over.. */

	cnt = 0;

	while ((len = fread(s_tmp_buf, 1, TMP_BUF_LEN, fh)) > 0) {
		cnt += len;
		printf("\rSending %6d/%6d of '%s'",cnt, (int)size, name);
		fflush(stdout);
	    if (send(s, s_tmp_buf, len, 0) != len) {
    	    ret = DT_ERR_CLIENT | DT_ERR_SEND;
        	goto lsg0_exit;
    	}
	}

	printf("\n");

	if (feof(fh) == 0) {
        ret = DT_ERR_CLIENT | DT_ERR_FILE_IO;
        goto lsg0_exit;
    }
    if (recv(s, &ret, 4, 0) != 4) {
        ret = DT_ERR_CLIENT | DT_ERR_RECV;
    } else {
        ret = ntohl(ret);
    }
lsg0_exit:
    if (hdr) {
        free(hdr);
    }

    fclose(fh);
    return ret;
}

static uint32_t lsg1(SOCK_T s, dt_options_t* p_opts,  char* name)
{
    return lsg0(s, p_opts, name);
}

static uint32_t adr0(SOCK_T s, dt_options_t* p_opts,  char* name)
{
    return lsg0(s, p_opts, name);
}

static uint32_t adf0(SOCK_T s, dt_options_t* p_opts,  char* name)
{
    return lsg0(s, p_opts, name);
}

static uint32_t peek(SOCK_T s, dt_options_t* p_opts,  char* name)
{
    return DT_ERR_CLIENT | DT_ERR_NOT_IMPLEMENTED;
}

static uint32_t quit_remote(SOCK_T s, dt_options_t* p_opts,  char* name)
{
    /* this plugin ignores the possible file */
    static char* command = DT_HDR_TAG_CMD_QUIT;
    uint32_t reply;
    int n;

    if (send(s,command,4,0) != 4) {
        return DT_ERR_CLIENT | DT_ERR_SEND;
    }
    if (recv(s,&reply,4,0) != 4) {
        return DT_ERR_CLIENT | DT_ERR_RECV;
    }

    return ntohl(reply);
}

static uint32_t reboot_remote(SOCK_T s, dt_options_t* p_opts, char* name)
{
    /* this plugin ignores the possible file */
    static char* command = DT_HDR_TAG_CMD_REBOOT;
    uint32_t reply;
    int n;

    if (send(s,command,4,0) != 4) {
        return DT_ERR_CLIENT | DT_ERR_SEND;
    }
    if (recv(s,&reply,4,0) != 4) {
        return DT_ERR_CLIENT | DT_ERR_RECV;
    }

    return ntohl(reply);
}

static plugin_t validate_command(char* name)
{
    /* oh right.. this could be more 'advanced' but.. ;) */
    if (!strcmp("lsg0",name)) {
        return lsg0;
    }
    if (!strcmp("lsg1",name)) {
        return lsg1;
    }
    if (!strcmp("adf0",name)) {
        return adf0;
    }
    if (!strcmp("adr0",name)) {
        return adr0;
    }
    if (!strcmp("peek",name)) {
        return peek;
    }

    /* commands without file transfer */
    if (!strcmp("quit",name)) {
        return quit_remote;
    }
    if (!strcmp("reboot",name)) {
        return reboot_remote;
    }

    return NULL;
}


static const struct option long_options[] = {
    {"version", required_argument,  0, 'v' },
    {"file",    required_argument,  0, 'f' },
    {"load",    required_argument,  0, 'l' },
    {"jump",    required_argument,  0, 'j' },
    {"reboot",  no_argument,        0, 'r' },
    {"exact",   no_argument,        0, 'e' },
    {"device",  required_argument,  0, 'd' },
    {"port",    required_argument,  0, 'P' },
    {"plugin",  required_argument,  0, 'p' },
    {"help",    no_argument,        0, 'h' },
    {0,0,0,0}
};

static dt_options_t s_opts = {0};

static void usage(char** argv)
{
    printf( "Usage:\n");
    printf( "  %s [options] command destination\n\n"
            "Commands are:\n"
            "  quit               Quit remote server.\n"
            "  reboot             Reboot remote Amiga.\n"
            "  lsg0               Execute 'file' using loadseg plugin.\n"
            "  lsg1               Execute 'file' using Internaloadsegplugin.\n"
            "  adr0               Execute 'file' using absolute adderess plugin.\n"
            "  adf0               Create a disk from 'file' using adf plugin.\n\n"
            "  peek               Dump memory to 'file' using peek plugin.\n\n"
            "Where [options] are:\n"
            "  --version,-v x.y   Minimum or exact plugin version, e.g. '1.2'.\n"
            "  --file,-f file     File to send to a remote Amiga or to save to\n"
            "                     from a remote Amiga.\n"
            "  --load,-l addr     Address to load 'file' into or address\n"
            "                     to read data from. Enter in hex. Some\n"
            "                     plugins can ignore this option.\n"
            "  --jump,-j addr     Address to start loaded 'file'. Enter in\n"
            "                     hex. Some plugins can ignore this option.\n"
            "  --reboot,-r        After running the 'file' reboot remote Amiga.\n"
            "  --exact,-e         Version match must be exact.\n"
            "  --device,-d name   Name of the device on Amiga. Some plugins can\n"
            "                     ignore this option. Max length is 15 characters.\n"
            "  --port,-p port     Port number of the server in a remote Amiga.\n"
            "                     Defaults to 9999.\n"
            "  --help,-h          This help output.\n\n"
            "Where inputs are:\n"
            "  destination        FQDN or IP-address of the remote server.\n"
            "",argv[0]);

    exit(1);
}


int main(int argc, char** argv)
{
    dt_header_t* hdr = NULL;
    SOCK_T sock;
    uint16_t port;

    int ch;
    uint32_t ret;
    int cmd;
    int hdr_len;
    plugin_t plugin;
    char *file;

    /* defaults */
    port = DT_DEF_PORT;
    s_opts.plugin = "LSG0";
    plugin = lsg0;
    s_opts.device = NULL;
    s_opts.addr = 0;
    s_opts.jump = 0;
    s_opts.size = 0;
    s_opts.major = 0;
    s_opts.minor = 0;
    file = NULL;

    /* check for options for plugins */
#ifdef __AMIGA__
	optind = 1;
	optreset = 1;
#endif

    while ((ch = getopt_long(argc, argv, "v:l:j:red:p:f:h", long_options, NULL)) != -1) {
        switch (ch) {
        case 'v':   // --version,-v
#ifndef __AMIGA__
            if (sscanf(optarg,"%d.%d",&s_opts.major,&s_opts.minor) != 2) {
#else
            if (sscanf(optarg,"%lu.%lu",&s_opts.major,&s_opts.minor) != 2) {
#endif
                usage(argv);
            }
            break;
        case 'f':   // --file,-f
            file = optarg;
            break;
        case 'l':   // --load,-l
            s_opts.addr = strtoul(optarg,NULL,0);
            break;
        case 'j':   // --jump,-j
            s_opts.jump = strtoul(optarg,NULL,0);
            break;
        case 'r':   // --reboot,-r
            s_opts.flags |= DT_FLG_REBOOT_ON_EXIT;
            break;
        case 'e':   // --exact,-e
            s_opts.flags |= DT_FLG_EXACT_VERSION;
            break;
        case 'p':   // --port,-p
            port = strtoul(optarg,NULL,0) & 0xffff;
            break;
        case 'd':   // --device,-d
            if (strlen(optarg) > 15) {
                usage(argv);
            }
            s_opts.flags |= DT_FLG_EXTENSION;
            s_opts.device = optarg;
            break;
        case 'h':
        default:
            usage(argv);
            break;
        }
    }

    /* enough mandatory parameters? */
    if (optind + 2 > argc) {
        usage(argv);
    }

    /* find the command.. */
    if ((plugin = validate_command(argv[optind])) == NULL) {
        usage(argv);
    }

    optind++;

#ifdef __AMIGA__
	/* Link the local program's 'errno' variable to the stack's
	 * internal 'errno' variable.
	 */
	SocketBaseTags(SBTM_SETVAL(SBTC_ERRNOPTR(sizeof(errno))),
		&errno, TAG_END);
#endif

    if ((sock = open_connection(argv[optind],port)) & DT_ERR_CLIENT) {
        printf("**Error: open_connection() failed with 0x%08x\n",sock);
        exit(1);
    }

    ret = plugin(sock,&s_opts,file);

    if (sock >= 0) {
#ifndef __AMIGA__
        close(sock);
#else
		CloseSocket(sock);
#endif
    }

    if (ret == 0) {
        return 0;
    } else {
        printf("**Error: command returned 0x%08lx.\n",ret);
        exit(1);
    }
}
