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

#define MSG_TYPE_CODEC(ACTION)                                                                          \
    ACTION( MSG_UNKNOWN )                                                                               \
    ACTION( MSG_REQ_MC_GET )                       /* memcache retrieval requests */                    \
    ACTION( MSG_REQ_MC_GETS )                                                                           \
    ACTION( MSG_REQ_MC_DELETE )                    /* memcache delete request */                        \
    ACTION( MSG_REQ_MC_CAS )                       /* memcache cas request and storage request */       \
    ACTION( MSG_REQ_MC_SET )                       /* memcache storage request */                       \
    ACTION( MSG_REQ_MC_ADD )                                                                            \
    ACTION( MSG_REQ_MC_REPLACE )                                                                        \
    ACTION( MSG_REQ_MC_APPEND )                                                                         \
    ACTION( MSG_REQ_MC_PREPEND )                                                                        \
    ACTION( MSG_REQ_MC_INCR )                      /* memcache arithmetic request */                    \
    ACTION( MSG_REQ_MC_DECR )                                                                           \
    ACTION( MSG_REQ_MC_QUIT )                      /* memcache quit request */                          \
    ACTION( MSG_RSP_MC_NUM )                       /* memcache arithmetic response */                   \
    ACTION( MSG_RSP_MC_STORED )                    /* memcache cas and storage response */              \
    ACTION( MSG_RSP_MC_NOT_STORED )                                                                     \
    ACTION( MSG_RSP_MC_EXISTS )                                                                         \
    ACTION( MSG_RSP_MC_NOT_FOUND )                                                                      \
    ACTION( MSG_RSP_MC_END )                                                                            \
    ACTION( MSG_RSP_MC_VALUE )                                                                          \
    ACTION( MSG_RSP_MC_DELETED )                   /* memcache delete response */                       \
    ACTION( MSG_RSP_MC_ERROR )                     /* memcache error responses */                       \
    ACTION( MSG_RSP_MC_CLIENT_ERROR )                                                                   \
    ACTION( MSG_RSP_MC_SERVER_ERROR )                                                                   \
    ACTION( MSG_REQ_REDIS_DEL )                    /* redis commands - keys */                          \
    ACTION( MSG_REQ_REDIS_EXISTS )                                                                      \
    ACTION( MSG_REQ_REDIS_EXPIRE )                                                                      \
    ACTION( MSG_REQ_REDIS_EXPIREAT )                                                                    \
    ACTION( MSG_REQ_REDIS_PEXPIRE )                                                                     \
    ACTION( MSG_REQ_REDIS_PEXPIREAT )                                                                   \
    ACTION( MSG_REQ_REDIS_PERSIST )                                                                     \
    ACTION( MSG_REQ_REDIS_PTTL )                                                                        \
    ACTION( MSG_REQ_REDIS_TTL )                                                                         \
    ACTION( MSG_REQ_REDIS_TYPE )                                                                        \
    ACTION( MSG_REQ_REDIS_APPEND )                 /* redis requests - string */                        \
    ACTION( MSG_REQ_REDIS_BITCOUNT )                                                                    \
    ACTION( MSG_REQ_REDIS_DECR )                                                                        \
    ACTION( MSG_REQ_REDIS_DECRBY )                                                                      \
    ACTION( MSG_REQ_REDIS_DUMP )                                                                        \
    ACTION( MSG_REQ_REDIS_GET )                                                                         \
    ACTION( MSG_REQ_REDIS_GETBIT )                                                                      \
    ACTION( MSG_REQ_REDIS_GETRANGE )                                                                    \
    ACTION( MSG_REQ_REDIS_GETSET )                                                                      \
    ACTION( MSG_REQ_REDIS_INCR )                                                                        \
    ACTION( MSG_REQ_REDIS_INCRBY )                                                                      \
    ACTION( MSG_REQ_REDIS_INCRBYFLOAT )                                                                 \
    ACTION( MSG_REQ_REDIS_MGET )                                                                        \
    ACTION( MSG_REQ_REDIS_PSETEX )                                                                      \
    ACTION( MSG_REQ_REDIS_RESTORE )                                                                     \
    ACTION( MSG_REQ_REDIS_SET )                                                                         \
    ACTION( MSG_REQ_REDIS_SETBIT )                                                                      \
    ACTION( MSG_REQ_REDIS_SETEX )                                                                       \
    ACTION( MSG_REQ_REDIS_SETNX )                                                                       \
    ACTION( MSG_REQ_REDIS_SETRANGE )                                                                    \
    ACTION( MSG_REQ_REDIS_STRLEN )                                                                      \
    ACTION( MSG_REQ_REDIS_HDEL )                   /* redis requests - hashes */                        \
    ACTION( MSG_REQ_REDIS_HEXISTS )                                                                     \
    ACTION( MSG_REQ_REDIS_HGET )                                                                        \
    ACTION( MSG_REQ_REDIS_HGETALL )                                                                     \
    ACTION( MSG_REQ_REDIS_HINCRBY )                                                                     \
    ACTION( MSG_REQ_REDIS_HINCRBYFLOAT )                                                                \
    ACTION( MSG_REQ_REDIS_HKEYS )                                                                       \
    ACTION( MSG_REQ_REDIS_HLEN )                                                                        \
    ACTION( MSG_REQ_REDIS_HMGET )                                                                       \
    ACTION( MSG_REQ_REDIS_HMSET )                                                                       \
    ACTION( MSG_REQ_REDIS_HSET )                                                                        \
    ACTION( MSG_REQ_REDIS_HSETNX )                                                                      \
    ACTION( MSG_REQ_REDIS_HVALS )                                                                       \
    ACTION( MSG_REQ_REDIS_LINDEX )                 /* redis requests - lists */                         \
    ACTION( MSG_REQ_REDIS_LINSERT )                                                                     \
    ACTION( MSG_REQ_REDIS_LLEN )                                                                        \
    ACTION( MSG_REQ_REDIS_LPOP )                                                                        \
    ACTION( MSG_REQ_REDIS_LPUSH )                                                                       \
    ACTION( MSG_REQ_REDIS_LPUSHX )                                                                      \
    ACTION( MSG_REQ_REDIS_LRANGE )                                                                      \
    ACTION( MSG_REQ_REDIS_LREM )                                                                        \
    ACTION( MSG_REQ_REDIS_LSET )                                                                        \
    ACTION( MSG_REQ_REDIS_LTRIM )                                                                       \
    ACTION( MSG_REQ_REDIS_RPOP )                                                                        \
    ACTION( MSG_REQ_REDIS_RPOPLPUSH )                                                                   \
    ACTION( MSG_REQ_REDIS_RPUSH )                                                                       \
    ACTION( MSG_REQ_REDIS_RPUSHX )                                                                      \
    ACTION( MSG_REQ_REDIS_SADD )                   /* redis requests - sets */                          \
    ACTION( MSG_REQ_REDIS_SCARD )                                                                       \
    ACTION( MSG_REQ_REDIS_SDIFF )                                                                       \
    ACTION( MSG_REQ_REDIS_SDIFFSTORE )                                                                  \
    ACTION( MSG_REQ_REDIS_SINTER )                                                                      \
    ACTION( MSG_REQ_REDIS_SINTERSTORE )                                                                 \
    ACTION( MSG_REQ_REDIS_SISMEMBER )                                                                   \
    ACTION( MSG_REQ_REDIS_SMEMBERS )                                                                    \
    ACTION( MSG_REQ_REDIS_SMOVE )                                                                       \
    ACTION( MSG_REQ_REDIS_SPOP )                                                                        \
    ACTION( MSG_REQ_REDIS_SRANDMEMBER )                                                                 \
    ACTION( MSG_REQ_REDIS_SREM )                                                                        \
    ACTION( MSG_REQ_REDIS_SUNION )                                                                      \
    ACTION( MSG_REQ_REDIS_SUNIONSTORE )                                                                 \
    ACTION( MSG_REQ_REDIS_ZADD )                   /* redis requests - sorted sets */                   \
    ACTION( MSG_REQ_REDIS_ZCARD )                                                                       \
    ACTION( MSG_REQ_REDIS_ZCOUNT )                                                                      \
    ACTION( MSG_REQ_REDIS_ZINCRBY )                                                                     \
    ACTION( MSG_REQ_REDIS_ZINTERSTORE )                                                                 \
    ACTION( MSG_REQ_REDIS_ZRANGE )                                                                      \
    ACTION( MSG_REQ_REDIS_ZRANGEBYSCORE )                                                               \
    ACTION( MSG_REQ_REDIS_ZRANK )                                                                       \
    ACTION( MSG_REQ_REDIS_ZREM )                                                                        \
    ACTION( MSG_REQ_REDIS_ZREMRANGEBYRANK )                                                             \
    ACTION( MSG_REQ_REDIS_ZREMRANGEBYSCORE )                                                            \
    ACTION( MSG_REQ_REDIS_ZREVRANGE )                                                                   \
    ACTION( MSG_REQ_REDIS_ZREVRANGEBYSCORE )                                                            \
    ACTION( MSG_REQ_REDIS_ZREVRANK )                                                                    \
    ACTION( MSG_REQ_REDIS_ZSCORE )                                                                      \
    ACTION( MSG_REQ_REDIS_ZUNIONSTORE )                                                                 \
    ACTION( MSG_REQ_REDIS_EVAL )                   /* redis requests - eval */                          \
    ACTION( MSG_REQ_REDIS_EVALSHA )                                                                     \
    ACTION( MSG_RSP_REDIS_STATUS )                 /* redis response */                                 \
    ACTION( MSG_RSP_REDIS_ERROR )                                                                       \
    ACTION( MSG_RSP_REDIS_INTEGER )                                                                     \
    ACTION( MSG_RSP_REDIS_BULK )                                                                        \
    ACTION( MSG_RSP_REDIS_MULTIBULK )                                                                   \
    ACTION( MSG_SENTINEL )                                                                              \


#define DEFINE_ACTION(_name) _name,
typedef enum msg_type {
    MSG_TYPE_CODEC(DEFINE_ACTION)
} msg_type_t;
#undef DEFINE_ACTION

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
    int64_t              start_ts;        /* request start timestamp in usec */

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
uint8_t *msg_type_str(msg_type_t type);

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
