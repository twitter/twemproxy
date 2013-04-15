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
typedef rstatus_t (*msg_post_splitcopy_t)(struct msg *);
typedef void (*msg_coalesce_t)(struct msg *r);

typedef enum msg_parse_result {
    MSG_PARSE_OK,                         /* parsing ok */
    MSG_PARSE_ERROR,                      /* parsing error */
    MSG_PARSE_REPAIR,                     /* more to parse -> repair parsed & unparsed data */
    MSG_PARSE_FRAGMENT,                   /* multi-vector request -> fragment */
    MSG_PARSE_AGAIN,                      /* incomplete -> parse again */
} msg_parse_result_t;

typedef enum msg_type {
    MSG_UNKNOWN,
    MSG_REQ_MC_GET,                       /* memcache retrieval requests */
    MSG_REQ_MC_GETS,
    MSG_REQ_MC_DELETE,                    /* memcache delete request */
    MSG_REQ_MC_CAS,                       /* memcache cas request and storage request */
    MSG_REQ_MC_SET,                       /* memcache storage request */
    MSG_REQ_MC_ADD,
    MSG_REQ_MC_REPLACE,
    MSG_REQ_MC_APPEND,
    MSG_REQ_MC_PREPEND,
    MSG_REQ_MC_INCR,                      /* memcache arithmetic request */
    MSG_REQ_MC_DECR,
    MSG_REQ_MC_QUIT,                      /* memcache quit request */
    MSG_RSP_MC_NUM,                       /* memcache arithmetic response */
    MSG_RSP_MC_STORED,                    /* memcache cas and storage response */
    MSG_RSP_MC_NOT_STORED,
    MSG_RSP_MC_EXISTS,
    MSG_RSP_MC_NOT_FOUND,
    MSG_RSP_MC_END,
    MSG_RSP_MC_VALUE,
    MSG_RSP_MC_DELETED,                   /* memcache delete response */
    MSG_RSP_MC_ERROR,                     /* memcache error responses */
    MSG_RSP_MC_CLIENT_ERROR,
    MSG_RSP_MC_SERVER_ERROR,
    MSG_REQ_REDIS_DEL,                    /* redis commands - keys */
    MSG_REQ_REDIS_EXISTS,
    MSG_REQ_REDIS_EXPIRE,
    MSG_REQ_REDIS_EXPIREAT,
    MSG_REQ_REDIS_PEXPIRE,
    MSG_REQ_REDIS_PEXPIREAT,
    MSG_REQ_REDIS_PERSIST,
    MSG_REQ_REDIS_PTTL,
    MSG_REQ_REDIS_TTL,
    MSG_REQ_REDIS_TYPE,
    MSG_REQ_REDIS_APPEND,                 /* redis requests - string */
    MSG_REQ_REDIS_BITCOUNT,
    MSG_REQ_REDIS_DECR,
    MSG_REQ_REDIS_DECRBY,
    MSG_REQ_REDIS_DUMP,
    MSG_REQ_REDIS_GET,
    MSG_REQ_REDIS_GETBIT,
    MSG_REQ_REDIS_GETRANGE,
    MSG_REQ_REDIS_GETSET,
    MSG_REQ_REDIS_INCR,
    MSG_REQ_REDIS_INCRBY,
    MSG_REQ_REDIS_INCRBYFLOAT,
    MSG_REQ_REDIS_MGET,
    MSG_REQ_REDIS_PSETEX,
    MSG_REQ_REDIS_RESTORE,
    MSG_REQ_REDIS_SET,
    MSG_REQ_REDIS_SETBIT,
    MSG_REQ_REDIS_SETEX,
    MSG_REQ_REDIS_SETNX,
    MSG_REQ_REDIS_SETRANGE,
    MSG_REQ_REDIS_STRLEN,
    MSG_REQ_REDIS_HDEL,                   /* redis requests - hashes */
    MSG_REQ_REDIS_HEXISTS,
    MSG_REQ_REDIS_HGET,
    MSG_REQ_REDIS_HGETALL,
    MSG_REQ_REDIS_HINCRBY,
    MSG_REQ_REDIS_HINCRBYFLOAT,
    MSG_REQ_REDIS_HKEYS,
    MSG_REQ_REDIS_HLEN,
    MSG_REQ_REDIS_HMGET,
    MSG_REQ_REDIS_HMSET,
    MSG_REQ_REDIS_HSET,
    MSG_REQ_REDIS_HSETNX,
    MSG_REQ_REDIS_HVALS,
    MSG_REQ_REDIS_LINDEX,                 /* redis requests - lists */
    MSG_REQ_REDIS_LINSERT,
    MSG_REQ_REDIS_LLEN,
    MSG_REQ_REDIS_LPOP,
    MSG_REQ_REDIS_LPUSH,
    MSG_REQ_REDIS_LPUSHX,
    MSG_REQ_REDIS_LRANGE,
    MSG_REQ_REDIS_LREM,
    MSG_REQ_REDIS_LSET,
    MSG_REQ_REDIS_LTRIM,
    MSG_REQ_REDIS_RPOP,
    MSG_REQ_REDIS_RPOPLPUSH,
    MSG_REQ_REDIS_RPUSH,
    MSG_REQ_REDIS_RPUSHX,
    MSG_REQ_REDIS_SADD,                   /* redis requests - sets */
    MSG_REQ_REDIS_SCARD,
    MSG_REQ_REDIS_SDIFF,
    MSG_REQ_REDIS_SDIFFSTORE,
    MSG_REQ_REDIS_SINTER,
    MSG_REQ_REDIS_SINTERSTORE,
    MSG_REQ_REDIS_SISMEMBER,
    MSG_REQ_REDIS_SMEMBERS,
    MSG_REQ_REDIS_SMOVE,
    MSG_REQ_REDIS_SPOP,
    MSG_REQ_REDIS_SRANDMEMBER,
    MSG_REQ_REDIS_SREM,
    MSG_REQ_REDIS_SUNION,
    MSG_REQ_REDIS_SUNIONSTORE,
    MSG_REQ_REDIS_ZADD,                   /* redis requests - sorted sets */
    MSG_REQ_REDIS_ZCARD,
    MSG_REQ_REDIS_ZCOUNT,
    MSG_REQ_REDIS_ZINCRBY,
    MSG_REQ_REDIS_ZINTERSTORE,
    MSG_REQ_REDIS_ZRANGE,
    MSG_REQ_REDIS_ZRANGEBYSCORE,
    MSG_REQ_REDIS_ZRANK,
    MSG_REQ_REDIS_ZREM,
    MSG_REQ_REDIS_ZREMRANGEBYRANK,
    MSG_REQ_REDIS_ZREMRANGEBYSCORE,
    MSG_REQ_REDIS_ZREVRANGE,
    MSG_REQ_REDIS_ZREVRANGEBYSCORE,
    MSG_REQ_REDIS_ZREVRANK,
    MSG_REQ_REDIS_ZSCORE,
    MSG_REQ_REDIS_ZUNIONSTORE,
    MSG_REQ_REDIS_EVAL,                   /* redis requests - eval */
    MSG_REQ_REDIS_EVALSHA,
    MSG_RSP_REDIS_STATUS,                 /* redis response */
    MSG_RSP_REDIS_ERROR,
    MSG_RSP_REDIS_INTEGER,
    MSG_RSP_REDIS_BULK,
    MSG_RSP_REDIS_MULTIBULK,
    MSG_SENTINEL
} msg_type_t;

struct msg {
    TAILQ_ENTRY(msg)     c_tqe;           /* link in client q */
    TAILQ_ENTRY(msg)     s_tqe;           /* link in server q */
    TAILQ_ENTRY(msg)     m_tqe;           /* link in send q / free q */

    uint64_t             id;              /* message id */
    struct msg           *peer;           /* message peer */
    struct conn          *owner;          /* message owner - client | server */

    struct rbnode        tmo_rbe;         /* entry in rbtree */

    struct mhdr          mhdr;            /* message mbuf header */
    uint32_t             mlen;            /* message length */

    int                  state;           /* current parser state */
    uint8_t              *pos;            /* parser position marker */
    uint8_t              *token;          /* token marker */

    msg_parse_t          parser;          /* message parser */
    msg_parse_result_t   result;          /* message parsing result */

    mbuf_copy_t          pre_splitcopy;   /* message pre-split copy */
    msg_post_splitcopy_t post_splitcopy;  /* message post-split copy */
    msg_coalesce_t       pre_coalesce;    /* message pre-coalesce */
    msg_coalesce_t       post_coalesce;   /* message post-coalesce */

    msg_type_t           type;            /* message type */

    uint8_t              *key_start;      /* key start */
    uint8_t              *key_end;        /* key end */

    uint32_t             vlen;            /* value length (memcache) */
    uint8_t              *end;            /* end marker (memcache) */

    uint8_t              *narg_start;     /* narg start (redis) */
    uint8_t              *narg_end;       /* narg end (redis) */
    uint32_t             narg;            /* # arguments (redis) */
    uint32_t             rnarg;           /* running # arg used by parsing fsa (redis) */
    uint32_t             rlen;            /* running length in parsing fsa (redis) */
    uint32_t             integer;         /* integer reply value (redis) */

    struct msg           *frag_owner;     /* owner of fragment message */
    uint32_t             nfrag;           /* # fragment */
    uint64_t             frag_id;         /* id of fragmented message */

    err_t                err;             /* errno on error? */
    unsigned             error:1;         /* error? */
    unsigned             ferror:1;        /* one or more fragments are in error? */
    unsigned             request:1;       /* request? or response? */
    unsigned             quit:1;          /* quit request? */
    unsigned             noreply:1;       /* noreply? */
    unsigned             done:1;          /* done? */
    unsigned             fdone:1;         /* all fragments are done? */
    unsigned             first_fragment:1;/* first fragment? */
    unsigned             last_fragment:1; /* last fragment? */
    unsigned             swallow:1;       /* swallow response? */
    unsigned             redis:1;         /* redis? */
};

TAILQ_HEAD(msg_tqh, msg);

struct msg *msg_tmo_min(void);
void msg_tmo_insert(struct msg *msg, struct conn *conn);
void msg_tmo_delete(struct msg *msg);

void msg_init(void);
void msg_deinit(void);
struct msg *msg_get(struct conn *conn, bool request, bool redis);
void msg_put(struct msg *msg);
struct msg *msg_get_error(bool redis, err_t err);
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
