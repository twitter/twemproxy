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

#ifndef _NC_MESSAGE_H_
#define _NC_MESSAGE_H_

#include <nc_core.h>

typedef void (*msg_parse_t)(struct msg *);

typedef enum msg_parse_result {
    MSG_PARSE_OK,           /* parsing ok */
    MSG_PARSE_ERROR,        /* parsing error */
    MSG_PARSE_REPAIR,       /* more to parse -> repair parsed & unparsed data */
    MSG_PARSE_FRAGMENT,     /* multi-get request -> fragment */
    MSG_PARSE_AGAIN,        /* incomplete -> parse again */
} msg_parse_result_t;

typedef enum msg_type {
    MSG_UNKNOWN,
    MSG_REQ_MC_GET,                     /* memcache retrieval requests */
    MSG_REQ_MC_GETS,
    MSG_REQ_MC_DELETE,                  /* memcache delete request */
    MSG_REQ_MC_CAS,                     /* memcache cas request and storage request */
    MSG_REQ_MC_SET,                     /* memcache storage request */
    MSG_REQ_MC_ADD,
    MSG_REQ_MC_REPLACE,
    MSG_REQ_MC_APPEND,
    MSG_REQ_MC_PREPEND,
    MSG_REQ_MC_INCR,                    /* memcache arithmetic request */
    MSG_REQ_MC_DECR,
    MSG_REQ_MC_QUIT,                    /* memcache quit request */
    MSG_RSP_MC_NUM,                     /* memcache arithmetic response */
    MSG_RSP_MC_STORED,                  /* memcache cas and storage response */
    MSG_RSP_MC_NOT_STORED,
    MSG_RSP_MC_EXISTS,
    MSG_RSP_MC_NOT_FOUND,
    MSG_RSP_MC_END,
    MSG_RSP_MC_VALUE,
    MSG_RSP_MC_DELETED,                 /* memcache delete response */
    MSG_RSP_MC_ERROR,                   /* memcache error responses */
    MSG_RSP_MC_CLIENT_ERROR,
    MSG_RSP_MC_SERVER_ERROR,
    MSG_SENTINEL
} msg_type_t;

struct msg {
    TAILQ_ENTRY(msg)   c_tqe;           /* link in client q */
    TAILQ_ENTRY(msg)   s_tqe;           /* link in server q */
    TAILQ_ENTRY(msg)   m_tqe;           /* link in send q / free q */

    uint64_t           id;              /* message id */
    struct msg         *peer;           /* message peer */
    struct conn        *owner;          /* message owner - client | server */

    struct rbnode      tmo_rbe;         /* entry in rbtree */

    struct mhdr        mhdr;            /* message mbuf header */
    uint32_t           mlen;            /* message length */

    int                state;           /* current parser state */
    uint8_t            *pos;            /* parser position marker */
    uint8_t            *token;          /* token marker */

    msg_parse_t        parser;          /* message parser */
    msg_parse_result_t result;          /* message parsing result */

    msg_type_t         type;            /* message type */
    uint8_t            *key_start;      /* key start */
    uint8_t            *key_end;        /* key end */

    uint32_t           vlen;            /* value length (memcache) */
    uint8_t            *end;            /* end marker (memcache) */

    uint64_t           frag_id;         /* id of fragmented message */
    err_t              err;             /* errno on error? */
    unsigned           error:1;         /* error? */
    unsigned           ferror:1;        /* one or more fragments are in error? */
    unsigned           request:1;       /* request? or response? */
    unsigned           quit:1;          /* quit request? */
    unsigned           noreply:1;       /* noreply? */
    unsigned           done:1;          /* done? */
    unsigned           fdone:1;         /* all fragments are done? */
    unsigned           last_fragment:1; /* last fragment of retrieval request? */
    unsigned           swallow:1;       /* swallow response? */
    unsigned           redis:1;         /* redis? */
};

TAILQ_HEAD(msg_tqh, msg);

struct msg *msg_tmo_min(void);
void msg_tmo_insert(struct msg *msg, struct conn *conn);
void msg_tmo_delete(struct msg *msg);

void msg_init(void);
void msg_deinit(void);
struct msg *msg_get(struct conn *conn, bool request, bool redis);
void msg_put(struct msg *msg);
struct msg *msg_get_error(err_t err);
void msg_dump(struct msg *msg);
bool msg_empty(struct msg *msg);
rstatus_t msg_recv(struct context *ctx, struct conn *conn);
rstatus_t msg_send(struct context *ctx, struct conn *conn);

struct msg *req_get(struct conn *conn);
void req_put(struct msg *msg);
bool req_done(struct conn *conn, struct msg *msg);
bool req_error(struct conn *conn, struct msg *msg);
void req_server_enqueue_imsgq(struct context *ctx, struct conn *conn, struct msg *msg);
void req_server_dequeue_imsgq(struct context *ctx, struct conn *conn, struct msg *msg);
void req_client_enqueue_omsgq(struct context *ctx, struct conn *conn, struct msg *msg);
void req_server_enqueue_omsgq(struct context *ctx, struct conn *conn, struct msg *msg);
void req_client_dequeue_omsgq(struct context *ctx, struct conn *conn, struct msg *msg);
void req_server_dequeue_omsgq(struct context *ctx, struct conn *conn, struct msg *msg);
struct msg *req_recv_next(struct context *ctx, struct conn *conn, bool alloc);
void req_recv_done(struct context *ctx, struct conn *conn, struct msg *msg, struct msg *nmsg);
struct msg *req_send_next(struct context *ctx, struct conn *conn);
void req_send_done(struct context *ctx, struct conn *conn, struct msg *msg);

struct msg *rsp_get(struct conn *conn);
void rsp_put(struct msg *msg);
struct msg *rsp_recv_next(struct context *ctx, struct conn *conn, bool alloc);
void rsp_recv_done(struct context *ctx, struct conn *conn, struct msg *msg, struct msg *nmsg);
struct msg *rsp_send_next(struct context *ctx, struct conn *conn);
void rsp_send_done(struct context *ctx, struct conn *conn, struct msg *msg);

#endif
