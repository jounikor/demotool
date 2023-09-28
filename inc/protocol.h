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
#ifndef _PROTOCOL_H_INCLUDED
#define _PROTOCOL_H_INCLUDED

/*
 * Note: network order (==big endian) is assumed.
 *
 * The socket communication fixed protocol parts:
 *
 *  3      2 2      1 1      0 0      0
 *  1      4 3      6 5      8 7      0
 * +--------+--------+--------+--------+
 * |  'H'      'D'      'R'   | lenver | i.e. taglenver
 * +--------+--------+--------+--------+
 *
 * or
 *
 *  3      2 2      1 1      0 0      0
 *  1      4 3      6 5      8 7      0
 * +--------+--------+--------+--------+
 * |  'Q'      'U'      'I'      'T'   | to quit the server
 * +--------+--------+--------+--------+
 *
 *  3      2 2      1 1      0 0      0
 *  1      4 3      6 5      8 7      0
 * +--------+--------+--------+--------+
 * |  'R'      'B'      'O'      'T'   | to reboot the remore Amiga
 * +--------+--------+--------+--------+
 *
 * lenver is coded as:
 *
 *   7 6 5 4 3 2 1 0
 *  +-+-+-+-+-+-+-+-+
 *  |  hdrlen   |ver| -> max size of the header is 256 bytes (0==256);
 *  +-+-+-+-+-+-+-+-+    hdrlen does not include taglenver.
 *
 * +--------+--------+--------+--------+
 * | requested plugin_id               | e.g. "LSG0"
 * +--------+--------+--------+--------+
 * | major  | minor  |    reserved     | e.g. 1.0 as the minimum version
 * +--------+--------+--------+--------+
 * | flags                             | e.g. PLUGIN_FLAG_REBOOT_ON_EXIT
 * +--------+--------+--------+--------+
 * | data length (recv/send)           |
 * +--------+--------+--------+--------+
 * | recv/send address (may be NULL)   |
 * +--------+--------+--------+--------+
 * | jump address (may be NULL)        |
 * +--------+--------+--------+--------+
 *
 * Extensions 0 or more:
 * 
 *  7      4 3      0     
 * +--------+--------+            +--------+
 * |  type  | length | + length * |  byte  |
 * +--------+--------+   0 to 15  +--------+
 *                       
 *
 * 1) Server to Client:
 *
 * +--------+--------+--------+--------+
 * | response                          | e.g. 0L for OK
 * +--------+--------+--------+--------+
 *
 * Plugin specific part follows:
 *
 * 1) Data to recv or send
 *
 * +--------+--------+--------+--------+
 * | data follows.. (not aligned)      |
 * :                                   :
 * +--------+--------+--------+--------+
 *
 * 2) Data receiver (client or server)
 *    sends a response after "data length"
 *    bytes have been received.
 *
 * +--------+--------+--------+--------+
 * | response                          | e.g. 0L for OK
 * +--------+--------+--------+--------+
 *
 * 3) response sender closes the connection first.
 *
 *
 */

typedef struct dt_header {
    char hdr_tag_len_ver[4];
    char plugin_tag_ver[4];
    uint8_t major;
    uint8_t minor;
    uint16_t reserved;
    uint32_t flags;
    uint32_t size;
    uint32_t addr;
    uint32_t jump;
    uint8_t  extension[];
} dt_header_t;  /* total 28 bytes for now */

#define DT_HDR_LEN_MASK 0xfc
#define DT_HDR_VER_MASK 0x03
#define DT_HDR_VERSION  0
#define DT_HDR_TAG "HDR"
#define DT_HDR_TAG_CMD_QUIT "QUIT"
#define DT_HDR_TAG_CMD_REBOOT "RBOT"

#define DT_CMD_PLUGIN   0   /* tag 'HDR' */
#define DT_CMD_QUIT     1   /* tag 'QUIT' */
#define DT_CMD_REBOOT   2   /* tag 'RBOT' */

#define DT_DEF_PORT             9999
#define DT_DEF_PLUGIN           "LSG0"
#define DT_DEF_CONNECT_TIMEOUT  5

/* These flags are also passed to the plugin init() */
#define DT_FLG_EXACT_VERSION    0x00000001  /* Exact version match required.
                                             * If not set then anything greater
                                             * or equal works..
                                             */

#define DT_FLG_REBOOT_ON_EXIT   0x00000002  /* Reboot when exiting done() */
#define DT_FLG_EXTENSION        0x00000004
#define DT_FLG_NO_RUN           0x00000008  /* do not run the loaded prg */

/* extentions Types */
#define DT_EXT_PADDING          0x00
#define DT_EXT_DEVICE_NAME      0x10

/* other extension related */
#define DT_EXT_MAX_LEN          15

/* Error codes */
#define DT_ERR_OK               0
#define DT_ERR_NOT_IMPLEMENTED  1

/* numbers from 1 to 999 are reserved for commands etc */
#define DT_ERR_MALLOC           1001
#define DT_ERR_OPENLIBRARY      1004
#define DT_ERR_INIT_FAILED      1005    /* generic failure */
#define DT_ERR_FILE_OPEN        1006
#define DT_ERR_FILE_IO          1007
#define DT_ERR_TAG_LEN_VER      1008    /* header was dud */
#define DT_ERR_NO_PLUGIN        1009
#define DT_ERR_EXEC             1010    /* executing the remote content failed */

#define DT_ERR_HDR_TAG          1011
#define DT_ERR_HDR_VERSION      1012
#define DT_ERR_HDR_LEN          1013    /* header is too short */
#define DT_ERR_HDR_INVALID      1014    /* invalid parameters */

#define DT_ERR_SOCKET_OPEN      2000
#define DT_ERR_RECV             2001
#define DT_ERR_SEND             2002
#define DT_ERR_CONNECT_TIMEOUT  2003
#define DT_ERR_FCNTL            2004
#define DT_ERR_DNS              2005
#define DT_ERR_SELECT           2006
#define DT_ERR_CONNECT          2007
#define DT_ERR_GETSOCKOPT       2008
#define DT_ERR_CONNECT_OTHER    2009


#define DT_ERR_CLIENT           0x80000000  /* add this to denote client side */


#endif /* _PROTOCOL_H_INCLUDED */