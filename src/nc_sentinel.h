#ifndef _NC_SENTINEL_H_
#define _NC_SENTINEL_H_

#include <nc_core.h>

#define SENTINEL_ADDR             "127.0.0.1"
#define SENTINEL_PORT             26379

#define SENTINEL_KEEPALIVE        30

#define SENTINEL_SERVERNAME_SPLIT '-'

#define INFO_SENTINEL "info sentinel\r\n"
#define SUB_SWITCH_REDIRECT "subscribe +switch-master +redirect-to-master\r\n"

#define SENTINEL_SWITCH_CHANNEL   "+switch-master"
#define SENTINEL_REDIRECT_CHANNEL "+redirect-to-master"

typedef enum sentinel_conn_status {
    SENTINEL_CONN_DISCONNECTED,
    SENTINEL_CONN_SEND_REQ,
    SENTINEL_CONN_ACK_INFO,
    SENTINEL_CONN_ACK_SWITCH_SUB,
    SENTINEL_CONN_ACK_REDIRECT_SUB,
} sentinel_conn_status_t;

struct conn * sentinel_conn(struct server *sentinel);
rstatus_t sentinel_connect(struct context *ctx, struct server *sentinel);
void sentinel_recv_done(struct context *ctx, struct conn *conn, struct msg *msg, struct msg *nmsg);
void sentinel_close(struct context *ctx, struct conn *conn);
int sentinel_reconnect(struct context *ctx, long long id, void *client_data);

#endif
