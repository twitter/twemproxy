#ifndef _NC_SENTINEL_H_
#define _NC_SENTINEL_H_

#include <nc_core.h>

#define SENTINEL_KEEP_INTERVAL        30

#define INFO_SENTINEL "info sentinel\r\n"
#define SUB_SWITCH_REDIRECT "subscribe +switch-master +redirect-to-master\r\n"

#define SENTINEL_SWITCH_CHANNEL   "+switch-master"
#define SENTINEL_REDIRECT_CHANNEL "+redirect-to-master"

struct conn * sentinel_conn(struct server *sentinel);
rstatus_t sentinel_connect(struct context *ctx, struct server *sentinel);
void sentinel_recv_done(struct context *ctx, struct conn *conn, struct msg *msg, struct msg *nmsg);
void sentinel_close(struct context *ctx, struct conn *conn);
int sentinel_reconnect(struct context *ctx, long long id, void *client_data);

#endif
