/**
 * 
 *
 *
 * A threadless event handler for non-blocking socket io.
 *
 *
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "event_handler.h"

size_t __stack=32768;

/*
 * User code starts
 *
 *
 *
 */


typedef struct eh_accept {
	long sock;
} eh_accept_t;

#define ACCEPT_HANDLER_NUM_LISTENERS 	4


/* user handlers */
int init_accept_handler(EVENT_HANDLER_CTX, eh_accept_t *p_ac, const in_addr_t *addr, in_port_t port);
int deinit_accept_handler(EVENT_HANDLER_CTX, eh_accept_t *p_ac);
void *accept_handler(EVENT_HANDLER_CTX, void *p_user);
void *read_handler(EVENT_HANDLER_CTX, void *p_user);


/* Disable immediate exit in case of CTRL-C */
void _chkabort(void) {
	//SetSignal(0L,SIGBREAKF_CTRL_C);
}

int init_accept_handler(EVENT_HANDLER_CTX, eh_accept_t *p_ac, const in_addr_t *addr, in_port_t port)
{
	struct sockaddr_in sin;
	long enable = 1;

	if ((p_ac->sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		return EH_ERROR_SOCKET_OPEN;
	}

	/* reuse addess and make socket non-blocking */
	if (setsockopt(p_ac->sock,SOL_SOCKET,SO_REUSEADDR,&enable,sizeof(enable)) != 0) {
		CloseSocket(p_ac->sock);
		return EH_ERROR_SOCKET_CONF;
	}

	/* bind it.. */
	memset(&sin, 0, sizeof(sin));

	sin.sin_family	= AF_INET;
	sin.sin_port	= htons(port);

	if (addr) {
		memcpy(&sin.sin_addr.s_addr, addr, sizeof(in_addr_t));
	} else {
		sin.sin_addr.s_addr	= INADDR_ANY;
	}

	if (bind(p_ac->sock, (struct sockaddr *)&sin, sizeof(sin)) != 0) {
		CloseSocket(p_ac->sock);
		return EH_ERROR_SOCKET_BIND;
	}

	if (listen(p_ac->sock, ACCEPT_HANDLER_NUM_LISTENERS) != 0) {
		CloseSocket(p_ac->sock);
		return EH_ERROR_SOCKET_LISTEN;
	}

	return EH_SUCCESS;
}

int deinit_accept_handler(EVENT_HANDLER_CTX, eh_accept_t *p_ac)
{
	CloseSocket(p_ac->sock);
	p_ac->sock = -1;
	return EH_SUCCESS;	
}

void *accept_handler(EVENT_HANDLER_CTX, void *p_user)
{
	long sock;
	eh_accept_t *p_ac = (eh_accept_t *)p_user;
	struct sockaddr_in from;
	long from_len = sizeof(from);

	sock = accept(p_ac->sock,(struct sockaddr *)&from,&from_len);

	if (sock == -1) {
		/* error case but continue with the same accept() handler */
		EVENT_HANDLER_RETURN(accept_handler);
	}

	/* START for demo only now.. */
	printf("-> From: %s:%u\n",
		Inet_NtoA(from.sin_addr.s_addr),
		ntohs(from.sin_port));

	eh_add_handler(EVENT_HANDLER_GET_CTX,
		EVENT_HANDLER_READ_TYPE,
		sock,
		read_handler,
		(void *)sock);

	EVENT_HANDLER_RETURN(accept_handler);
}


void *read_handler(EVENT_HANDLER_CTX, void *p_user)
{
	long sock = (long)p_user;
	char buf[256+1];
	int r;
	
	r = recv(sock,buf,256,0);

	if (r == 0) {
		/* all done */
		CloseSocket(sock);
		EVENT_HANDLER_RETURN(NULL);
	}

	if (r < 0) {	
		if (Errno() == EAGAIN) {
			EVENT_HANDLER_RETURN(read_handler);
		}
	
		printf("\n**ERROR %d ABorting **\n",r);
		CloseSocket(sock);
		return NULL;
	}

	buf[r] = '\0';
	printf("%s",buf);

	EVENT_HANDLER_RETURN(read_handler);
}

/*
 *
 *
 *
 *
 */
static event_handler_t ctx;

int main(int argc, char **argv)
{
	int r;
	eh_accept_t ac;

	r = eh_init_with_open(&ctx);
	//r = eh_init_with_open(CHECK_CTX(ctx));

	if (r != EH_SUCCESS) {
		printf("**Error: eh_init_with_open() returned %d\n",r);
		return 0;
	}
	r = init_accept_handler(&ctx,&ac,NULL,9999);

	if (r != EH_SUCCESS) {
		printf("**Error: init_accept_handler() returned %d\n",r);
		eh_deinit(&ctx);
		return 0;
	}

	r = eh_add_handler(&ctx,
			EVENT_HANDLER_READ_TYPE,
			ac.sock,
			accept_handler,
			&ac);

	if (r != EH_SUCCESS) {
		printf("**Error: eh_add_handler() returned %d\n",r);
		deinit_accept_handler(&ctx,&ac);
		eh_deinit(&ctx);
		return 0;
	}
	
	eh_run(&ctx);

	printf("1\n");
	deinit_accept_handler(&ctx,&ac);

	printf("2\n");
	eh_deinit(&ctx);

}
