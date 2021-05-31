/*
 * twemproxy - A fast and lightweight proxy for memcached protocol.
 * Copyright (C) 2011 Twitter, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _NC_CONNECTION_H_
#define _NC_CONNECTION_H_

#include <nc_core.h>

typedef rstatus_t (*conn_recv_t)(struct context *, struct conn*);
typedef struct msg* (*conn_recv_next_t)(struct context *, struct conn *, bool);
typedef void (*conn_recv_done_t)(struct context *, struct conn *, struct msg *, struct msg *);

typedef rstatus_t (*conn_send_t)(struct context *, struct conn*);
typedef struct msg* (*conn_send_next_t)(struct context *, struct conn *);
typedef void (*conn_send_done_t)(struct context *, struct conn *, struct msg *);

typedef void (*conn_close_t)(struct context *, struct conn *);
typedef bool (*conn_active_t)(struct conn *);

typedef void (*conn_ref_t)(struct conn *, void *);
typedef void (*conn_unref_t)(struct conn *);

typedef void (*conn_msgq_t)(struct context *, struct conn *, struct msg *);
typedef void (*conn_post_connect_t)(struct context *ctx, struct conn *, struct server *server);
typedef void (*conn_swallow_msg_t)(struct conn *, struct msg *, struct msg *);

/*
 * There are few kinds of connections, client-bound, server-bound, redis,
 * memcache, proxy, etc. We uniquely identify a connection kind using a single
 * enum, otherwise the code becomes a tangle of if's.
 */
enum conn_kind {
    NC_CONN_CLIENT_MEMCACHE,
    NC_CONN_CLIENT_REDIS,
    NC_CONN_SERVER_MEMCACHE,
    NC_CONN_SERVER_REDIS,
    NC_CONN_PROXY_MEMCACHE,
    NC_CONN_PROXY_REDIS,
    NC_CONN_SIGNAL,
    _NC_CONN_KIND_MAX
};
static const char *const conn_kind_s[] = {
    "client:memcache",
    "client:redis",
    "server:memcache",
    "server:redis",
    "proxy:memcache",
    "proxy:redis",
    "signal"
};
#define CONN_KIND_AS_STRING(c)  (conn_kind_s[(c)->conn_kind])
#define CONN_KIND_IS_REDIS(c)   (((c)->conn_kind) & 1)
#define CONN_KIND_IS_CLIENT(c)  ((c)->conn_kind <= NC_CONN_CLIENT_REDIS)
#define CONN_KIND_IS_SERVER(c)  ((c)->conn_kind >= NC_CONN_SERVER_MEMCACHE \
                              && (c)->conn_kind <= NC_CONN_SERVER_REDIS)
#define CONN_KIND_IS_PROXY(c)   ((c)->conn_kind >= NC_CONN_PROXY_MEMCACHE \
                              && (c)->conn_kind <= NC_CONN_PROXY_REDIS)

struct conn {
    TAILQ_ENTRY(conn)   conn_tqe;        /* link in server_pool / server / free q */
    void                *owner;          /* connection owner - server_pool / server */

    int                 sd;              /* socket descriptor */
    struct sockinfo     info;            /* socket address */

    struct msg_tqh      imsg_q;          /* incoming request Q */
    struct msg_tqh      omsg_q;          /* outstanding request Q */
    struct msg          *rmsg;           /* current message being rcvd */
    struct msg          *smsg;           /* current message being sent */

    conn_recv_t         recv;            /* recv (read) handler */
    conn_recv_next_t    recv_next;       /* recv next message handler */
    conn_recv_done_t    recv_done;       /* read done handler */
    conn_send_t         send;            /* send (write) handler */
    conn_send_next_t    send_next;       /* write next message handler */
    conn_send_done_t    send_done;       /* write done handler */
    conn_close_t        close;           /* close handler */
    conn_active_t       active;          /* active? handler */
    conn_post_connect_t post_connect;    /* post connect handler */
    conn_swallow_msg_t  swallow_msg;     /* react on messages to be swallowed */

    conn_ref_t          ref;             /* connection reference handler */
    conn_unref_t        unref;           /* connection unreference handler */

    conn_msgq_t         enqueue_inq;     /* connection inq msg enqueue handler */
    conn_msgq_t         dequeue_inq;     /* connection inq msg dequeue handler */
    conn_msgq_t         enqueue_outq;    /* connection outq msg enqueue handler */
    conn_msgq_t         dequeue_outq;    /* connection outq msg dequeue handler */

    size_t              recv_bytes;      /* received (read) bytes */
    size_t              send_bytes;      /* sent (written) bytes */

    uint32_t            events;          /* connection io events */
    err_t               err;             /* connection errno */
    unsigned            recv_active:1;   /* recv active? */
    unsigned            recv_ready:1;    /* recv ready? */
    unsigned            send_active:1;   /* send active? */
    unsigned            send_ready:1;    /* send ready? */

    enum conn_kind      conn_kind:3;     /* 2^3 <= _NC_CONN_KIND_MAX */
    unsigned            connecting:1;    /* connecting? */
    unsigned            connected:1;     /* connected? */
    unsigned            eof:1;           /* eof? aka passive close? */
    unsigned            done:1;          /* done? aka close? */
    unsigned            authenticated:1; /* authenticated? */
};

TAILQ_HEAD(conn_tqh, conn);

struct context *conn_to_ctx(struct conn *conn);
struct conn *conn_get(void *owner, enum conn_kind);
enum conn_kind conn_get_kind(struct conn *);
void conn_put(struct conn *conn);
ssize_t conn_recv(struct conn *conn, void *buf, size_t size);
ssize_t conn_sendv(struct conn *conn, struct array *sendv, size_t nsend);
void conn_init(void);
void conn_deinit(void);
uint32_t conn_ncurr_conn(void);
uint64_t conn_ntotal_conn(void);
uint32_t conn_ncurr_cconn(void);
bool conn_authenticated(struct conn *conn);
/* Return formatted local end of the connection for proxy conn;
 * remote address for server and client. */
char *conn_unresolve_descriptive(struct conn *);

#endif
