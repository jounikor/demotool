#include <proto/exec.h>
#include <proto/bsdsocket.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include "event_handler.h"

/* private event handler functions */
static handler_info_t *eh_find_handler(const EVENT_HANDLER_CTX, int type, long sock);
static int eh_handle_callback(EVENT_HANDLER_CTX, fd_set *set, handler_info_t *p_info);
static long eh_handle_fdset(fd_set *set, handler_info_t *p_info, long max);

/*
 * @brief Initialize the event handler listening socket.
 *
 *
 *
 *
 */
int eh_init_with_open(event_handler_t *ctx)
{
	/* basic init involve proper value for 'SocketBase' */
	ctx->socket_base = OpenLibrary(
							EVENT_HANDLER_LIBRARY_NAME,
							EVENT_HANDLER_LIBRARY_VERSION);

	if (ctx->socket_base == NULL) {
		return EH_ERROR_OPEN_LIBRARY;
	}

	/* prepare copies of the fd_sets */
	for (int n = 0; n < FD_SETSIZE; n++) {
		ctx->rset[n].func = NULL;
		ctx->rset[n].sock = -1;
		ctx->rset[n].p_user = NULL;

		ctx->wset[n].func = NULL;
		ctx->wset[n].sock = -1;
		ctx->wset[n].p_user = NULL;

		ctx->eset[n].func = NULL;
		ctx->eset[n].sock = -1;
		ctx->eset[n].p_user = NULL;
	}

	return EH_SUCCESS;
}

int eh_deinit(event_handler_t *ctx)
{
	if (ctx->socket_base) {
		CloseLibrary(ctx->socket_base);
		ctx->socket_base = NULL;
	}

	return EH_SUCCESS;
}


static handler_info_t *eh_find_handler(const EVENT_HANDLER_CTX, int type, long sock)
{
	handler_info_t *p_info;

	if (sock < 0 || sock >= FD_SETSIZE) {	
		return NULL;
	}

	switch (type) {
	case EVENT_HANDLER_READ_TYPE:
		p_info= &EVENT_HANDLER_GET_CTX->rset[sock];
		break;
	case EVENT_HANDLER_SEND_TYPE:
		p_info= &EVENT_HANDLER_GET_CTX->wset[sock];
		break;
	case EVENT_HANDLER_EXCP_TYPE:
		p_info= &EVENT_HANDLER_GET_CTX->eset[sock];
		break;
	default:
		return NULL;
	}

	return p_info;
}


int eh_add_handler(EVENT_HANDLER_CTX, int type, long sock, event_handler_function_t func, void *p_user)
{
	handler_info_t *p_info = eh_find_handler(EVENT_HANDLER_GET_CTX,type,sock);
	int ret_val = EH_SUCCESS;

	if (p_info == NULL) {
		return EH_ERROR_HANDLER_BOUNDS;
	}

	long enable = 1;

	if (IoctlSocket(sock, FIONBIO, &enable) != 0) {
		CloseSocket(sock);
		return EH_ERROR_SOCKET_CONF;
	}

	if (p_info->func) {
		ret_val = EH_HANDLER_REPLACED;
	}

	p_info->sock = sock;
	p_info->func = func;
	p_info->p_user = p_user;

	return EH_SUCCESS;
}

int eh_del_handler(EVENT_HANDLER_CTX, int type, long sock)
{
	handler_info_t *p_info = eh_find_handler(EVENT_HANDLER_GET_CTX,type,sock);

	if (p_info == NULL) {
		return EH_ERROR_HANDLER_BOUNDS;
	}

	if (p_info->func == NULL) {
		return EH_ERROR_HANDLER_NOTFOUND;
	}

	if (p_info->sock != sock) {
		return EH_ERROR_HANDLER_SOCKET;
	}

	p_info->func = NULL;
	p_info->sock = -1;
	p_info->p_user = NULL;

	return EH_SUCCESS;
}

int eh_reset_handler(EVENT_HANDLER_CTX, int type, long sock)
{
	handler_info_t *p_info = eh_find_handler(EVENT_HANDLER_GET_CTX,type,sock);

	if (p_info == NULL) {
		return EH_ERROR_HANDLER_BOUNDS;
	}

	p_info->func = NULL;
	p_info->sock = -1;
	p_info->p_user = NULL;

	return EH_SUCCESS;
}


int eh_add_timer(EVENT_HANDLER_CTX, struct timeval *tout, event_handler_function_t func, void *p_user)
{
	return EH_SUCCESS;
}

int eh_del_timer(EVENT_HANDLER_CTX, struct timeval *tout, event_handler_function_t func)
{
	return EH_SUCCESS;
}


static int eh_handle_callback(EVENT_HANDLER_CTX, fd_set *set, handler_info_t *p_info)
{
	void *func;
	int r;

	if (p_info->sock != -1) {
		if (FD_ISSET(p_info->sock,set)) {
			func = p_info->func(EVENT_HANDLER_GET_CTX, p_info->p_user);

			if (func == NULL) {
				eh_del_handler(EVENT_HANDLER_GET_CTX,EVENT_HANDLER_READ_TYPE,p_info->sock);
			} else {
				/* for this socket replace the hander with a new one */
				p_info->func = func;
			}
		}
	}

	return EH_SUCCESS;
}

static long eh_handle_fdset(fd_set *set, handler_info_t *p_info, long max)
{
	if (p_info->sock != -1) {
		FD_SET(p_info->sock,set);
			
		if (p_info->sock >= max) {
			max = p_info->sock + 1;
		}
	}

	return max;
}


int eh_wfe(EVENT_HANDLER_CTX)
{
	long max = -1;
	int n, r;
	struct timeval tv;

	fd_set rset;
	fd_set wset;
	fd_set eset;

	FD_ZERO(&rset);
	FD_ZERO(&wset);
	FD_ZERO(&eset);

	for (n = 0; n < FD_SETSIZE; n++) {
		max= eh_handle_fdset(&rset, &EVENT_HANDLER_GET_CTX->rset[n], max);
		max= eh_handle_fdset(&wset, &EVENT_HANDLER_GET_CTX->wset[n], max);
		max= eh_handle_fdset(&eset, &EVENT_HANDLER_GET_CTX->eset[n], max);
	}

	tv.tv_sec = EVENT_HANDLER_TIMEOUT;
	tv.tv_usec = 0;

	r = WaitSelect(max,&rset,&wset,&eset,&tv,NULL);

	if (r == 0) {
		/* timeout */
		return EH_SUCCESS;
	} else if (r < 0) {
		return EH_ERROR_HANDLER_BREAK;
	}

	for (n = 0; n < max; n++) {
		if (eh_handle_callback(EVENT_HANDLER_GET_CTX,&rset,&EVENT_HANDLER_GET_CTX->rset[n]));
		if (eh_handle_callback(EVENT_HANDLER_GET_CTX,&wset,&EVENT_HANDLER_GET_CTX->wset[n]));
		if (eh_handle_callback(EVENT_HANDLER_GET_CTX,&eset,&EVENT_HANDLER_GET_CTX->eset[n]));
	}

	return EH_SUCCESS;
}



int eh_run(event_handler_t *ctx)
{
	int ret_val;

	do {
		ret_val = eh_wfe(ctx);
	} while (ret_val == EH_SUCCESS);

	/* for demo purposes */
	if (ret_val == EH_ERROR_HANDLER_BREAK) {
	}

	return ret_val;
}
