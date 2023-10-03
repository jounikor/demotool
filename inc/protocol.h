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
 * FROM CLIENT TO SERVER =================================>
 *
 *  3      2 2      1 1      0 0      0
 *  1      4 3      6 5      8 7      0
 * +--------+--------+--------+--------+
 * |  'Q'      'U'      'I'      'T'   | to quit the server
 * +--------+--------+--------+--------+
 *
 * or..
 *
 *  3      2 2      1 1      0 0      0
 *  1      4 3      6 5      8 7      0
 * +--------+--------+--------+--------+
 * |  'R'      'B'      'O'      'T'   | to reboot the server
 * +--------+--------+--------+--------+
 *
 * or..
 *
 *  3      2 2      1 1      0 0      0
 *  1      4 3      6 5      8 7      0
 * +--------+--------+--------+--------+
 * |  'H'      'D'      'R'   | lenver | to exchange a file with the client
 * +--------+--------+--------+--------+
 * :                                   :
 *    Followed by the fixed header:
 * :                                   :
 * +--------+--------+--------+--------+
 * | requested plugin_id               | e.g. "lsg0"
 * +--------+--------+--------+--------+
 * | major  | minor  |    reserved     | e.g. 1.0 as the minimum version
 * +--------+--------+--------+--------+
 * | flags                             | e.g. DT_FLG_REBOOT_ON_EXIT
 * +--------+--------+--------+--------+
 * | data length (recv/send)           |
 * +--------+--------+--------+--------+
 * | recv/send address (may be NULL)   |
 * +--------+--------+--------+--------+
 * | jump address (may be NULL)        |
 * +--------+--------+--------+--------+
 * : 0 or more octets of extensions..  :
 * +-----------------------------------+
 *
 * Extensions are encoded as:
 * 
 *  7      4 3      0     
 * +--------+--------+            +--------+
 * |  type  | length | + length * |  byte  |
 * +--------+--------+   0 to 15  +--------+
 *                       
 * lenver is coded as:
 *
 *   7 6 5 4 3 2 1 0
 *  +-+-+-+-+-+-+-+-+
 *  |  hdrlen   |ver| -> max size of the header is 256 bytes (0==256);
 *  +-+-+-+-+-+-+-+-+    hdrlen does not include taglenver.
 *
 * SERVER TO CLIENT RESPONSES FOR COMMANDS <===============
 *
 * +--------+--------+--------+--------+
 * | response                          | e.g. DT_ERR_OK etc
 * +--------+--------+--------+--------+
 *
 * PLUGIN CLIENT TO SERVER TO CLIENT PROTOCOLS <==========>
 *
 * The protocol is _plugin_ specific.. You just need to make
 * sure the plugin and the client for the matching plugin
 * implement the same behavior.
 *
 * See existing plugins for examples. Note that currently 
 * LSG0, ADF0 and ADR0 plugins all share the same "protocol"
 * i.e., the client sends 'size' bytes to the server and the
 * server responses with 4 byte acknowledgement.
 *
 * The PEEK plugin just sends 'size' bytes from the server
 * to the client. No acknowledgements are used.
 *
 * Another implementation note. If the client fails to send()
 * data to the server it should try to read back the 4 byte
 * acknowledgement as it likely contains the error reason.
 */

/**
 * @struct dt_header protocol.h
 * @typedef struct dt_header dt_header_t
 *
 * The fixed header part for the client to server handshake.
 */
typedef struct dt_header {
    char hdr_tag_len_ver[4];	/**< command or header+len+ver */
    char plugin_tag_ver[4];		/**< requested plugin tag */
    uint8_t major;				/**< plugin version major */
    uint8_t minor;				/**< plugin version minor */
    uint16_t reserved;
    uint32_t flags;				/**< flags guiding plugin handling */
    uint32_t size;				/**< size of the data payload */
    uint32_t addr;				/**< load address to/from server memory */
    uint32_t jump;				/**< execution address for absolute exes */
    uint8_t  extension[];		/**< optional extensions */
} dt_header_t;  /* total 28 bytes for now */

/*  Defines for dt_header_t hdr_tag_len_ver content and handling. */
#define DT_HDR_LEN_MASK 0xfc
#define DT_HDR_VER_MASK 0x03
#define DT_HDR_VERSION  0
#define DT_HDR_TAG "HDR"
#define DT_HDR_TAG_CMD_QUIT "QUIT"
#define DT_HDR_TAG_CMD_REBOOT "RBOT"

#define DT_CMD_PLUGIN   0   /* tag 'HDR' */
#define DT_CMD_QUIT     1   /* tag 'QUIT' */
#define DT_CMD_REBOOT   2   /* tag 'RBOT' */
#define DT_CMD_ERROR	0x80000000

/* Defaults for the server */
#define DT_DEF_PORT             9999
#define DT_DEF_PLUGIN           "LSG0"
#define DT_DEF_CONNECT_TIMEOUT  5

/* These flags are also passed to the plugin init() */
#define DT_FLG_EXACT_VERSION    0x00000001  /**< Exact version match required.
                                             *   If not set then anything greater
                                             *   or equal works..
                                             */

#define DT_FLG_REBOOT_ON_EXIT   0x00000002  /**< Reboot when exiting done() */
#define DT_FLG_EXTENSION        0x00000004	/**< The header has extensions */
#define DT_FLG_NO_RUN           0x00000008  /**< Do not run the loaded prg */

/* extentions Types */
#define DT_EXT_PADDING          0x00		/**< 1 byte padding */
#define DT_EXT_DEVICE_NAME      0x10		/**< device name such */

/* other extension related */
#define DT_EXT_MAX_LEN          15			/**< Max extension length. Strings do
											 *   not have NUL termination
                                             */ 
/* Error codes */
#define DT_ERR_OK               0
#define DT_ERR_NOT_IMPLEMENTED  1

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

#define DT_ERR_CREATE_MSGPORT	4000
#define DT_ERR_CREATE_IOREQ		4001
#define DT_ERR_OPENDEVICE		4002
#define DT_ERR_DISK_DEVICE		4003
#define DT_ERR_DISK_IO			4096	/* + device error */


#define DT_ERR_CLIENT           0x80000000  /* add this to denote client side */


#endif /* _PROTOCOL_H_INCLUDED */
