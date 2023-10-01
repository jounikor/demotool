#ifndef _EVENT_HANDLER_H_INCLUDED
#define _EVENT_HANDLER_H_INCLUDED

#include <proto/exec.h>
#include <exec/types.h>
#include <devices/timer.h>

/* important not to define the library base implicitly */
#define NO_INLINE_STDARG
#define __NOLIBBASE__

#include <proto/bsdsocket.h>

/**
 * @brief Internal state structure for the event handler.
 *
 */

typedef struct event_handler event_handler_t;
typedef void* (*event_handler_function_t)(event_handler_t *, void *);

typedef struct handler_info {
	long sock;
	event_handler_function_t func;
	void *p_user;
} handler_info_t;

struct event_handler {
	struct Library *socket_base;

	/* naive tables for storing handler datas */
	handler_info_t rset[FD_SETSIZE];
	handler_info_t wset[FD_SETSIZE];
	handler_info_t eset[FD_SETSIZE];
	/* TODO timer handler support */
};

/* The following is a bit ugly construction but the user handlers
 * in general should not know anything about the event handler
 * context structure..
 */
#define EVENT_HANDLER_GET_CTX		__eh

/* macro for user handler entry and generic handler context */

#define LOCAL_VAR &
#define CHECK_CTX(x) __typeof(x) == __typeof(event_handler_t *) ? (x) : LOCAL_VAR (x)


#define EVENT_HANDLER_CTX			\
	event_handler_t *EVENT_HANDLER_GET_CTX

#define EVENT_HANDLER_DEF_CTX(x)	\
	sizeof(x) == sizeof(event_handler_t *) ? x : x

/* macro for user handler return */
#define EVENT_HANDLER_RETURN(f)	\
	return (void *)f

/* macro to load the SocketBase in a local context */
#define SocketBase EVENT_HANDLER_GET_CTX->socket_base

/* bsdlibrary stuff */
#define EVENT_HANDLER_LIBRARY_VERSION 	4
#define EVENT_HANDLER_LIBRARY_NAME 		"bsdsocket.library"
#define EVENT_HANDLER_TIMEOUT			5

/* handler types */
#define EVENT_HANDLER_READ_TYPE 0
#define EVENT_HANDLER_SEND_TYPE 1
#define EVENT_HANDLER_EXCP_TYPE 2
#define EVENT_HANDLER_TIMR_TYPE 3		/* TODO */

/* event handler return codes */
#define EH_SUCCESS				0
#define EH_HANDLER_REPLACED		1

#define EH_ERROR_OPEN_LIBRARY	-1
#define EH_ERROR_SOCKET_OPEN	-2
#define EH_ERROR_SOCKET_CONF	-3
#define EH_ERROR_SOCKET_BIND	-4
#define EH_ERROR_SOCKET_LISTEN	-5
#define EH_ERROR_SOCKET_ACCEPT	-6

#define EH_ERROR_HANDLER_NOTFOUND	-108
#define EH_ERROR_HANDLER_EXISTS		-109
#define EH_ERROR_HANDLER_SOCKET		-110
#define EH_ERROR_HANDLER_BOUNDS		-111

#define EH_ERROR_HANDLER_BREAK		-112

/* event handler exported functions for init/deinit */
int eh_init_with_open(event_handler_t *ctx);
int eh_deinit(event_handler_t *ctx);
int eh_run(event_handler_t *ctx);
int eh_wfe(EVENT_HANDLER_CTX);

/* generic exported handler mnagement functions */
int eh_add_handler(EVENT_HANDLER_CTX, int type, long sock, event_handler_function_t func, void *p_user);
int eh_del_handler(EVENT_HANDLER_CTX, int type, long sock);
int eh_reset_handler(EVENT_HANDLER_CTX, int type, long sock);
int eh_add_timer(EVENT_HANDLER_CTX, struct timeval *tout, event_handler_function_t func, void *p_user);
int eh_del_timer(EVENT_HANDLER_CTX, struct timeval *tout, event_handler_function_t func);

#endif	/* EVENT_HANDLER_H_INCLUDED */