#ifndef __bot_ssocket_h__
#define __bot_ssocket_h__

/**
 * SECTION:ssocket
 * @title:TCP Sockets
 * @short_description:
 * @include: bot2-core/bot2-core.h
 *
 * TODO
 *
 * Linking: -lbot2-core
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bot_ssocket bot_ssocket_t;

struct bot_ssocket
{
	int type;
	int socket;

	struct sockaddr addr;
	socklen_t addrlen;

};

bot_ssocket_t *bot_ssocket_create(void);
void bot_ssocket_destroy(bot_ssocket_t *s);
int bot_ssocket_connect(bot_ssocket_t *s, const char *hostname, int port);
int bot_ssocket_disable_nagle(bot_ssocket_t *s);
int bot_ssocket_listen(bot_ssocket_t *s, int port, int listenqueue, int localhostOnly);
bot_ssocket_t *bot_ssocket_accept(bot_ssocket_t *s);
void bot_ssocket_get_remote_ip(bot_ssocket_t *s, int *ip);
int bot_ssocket_get_fd(bot_ssocket_t *s);

#ifdef __cplusplus
}
#endif

#endif
