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

#ifndef _NC_STATS_H_
#define _NC_STATS_H_

#include <nc_core.h>
#include <nc_event.h>

#define STATS_POOL_CODEC(ACTION)                        \
    /* client behavior */                               \
    ACTION( client_eof,             STATS_COUNTER )     \
    ACTION( client_err,             STATS_COUNTER )     \
    ACTION( client_connections,     STATS_GAUGE   )     \
    /* pool behavior */                                 \
    ACTION( server_ejects,          STATS_COUNTER )     \
    /* forwarder behavior */                            \
    ACTION( forward_error,          STATS_COUNTER )     \
    ACTION( fragments,              STATS_COUNTER )     \

#define STATS_SERVER_CODEC(ACTION)                      \
    /* msg (request) type */                            \
    ACTION( get,                    STATS_COUNTER )     \
    ACTION( gets,                   STATS_COUNTER )     \
    ACTION( delete,                 STATS_COUNTER )     \
    ACTION( cas,                    STATS_COUNTER )     \
    ACTION( set,                    STATS_COUNTER )     \
    ACTION( add,                    STATS_COUNTER )     \
    ACTION( replace,                STATS_COUNTER )     \
    ACTION( append,                 STATS_COUNTER )     \
    ACTION( prepend,                STATS_COUNTER )     \
    ACTION( incr,                   STATS_COUNTER )     \
    ACTION( decr,                   STATS_COUNTER )     \
    ACTION( noreply,                STATS_COUNTER )     \
    /* msg (response) type */                           \
    ACTION( num,                    STATS_COUNTER )     \
    ACTION( stored,                 STATS_COUNTER )     \
    ACTION( not_stored,             STATS_COUNTER )     \
    ACTION( exists,                 STATS_COUNTER )     \
    ACTION( not_found,              STATS_COUNTER )     \
    ACTION( value,                  STATS_COUNTER )     \
    ACTION( end,                    STATS_COUNTER )     \
    ACTION( deleted,                STATS_COUNTER )     \
    ACTION( error,                  STATS_COUNTER )     \
    ACTION( client_error,           STATS_COUNTER )     \
    ACTION( server_error,           STATS_COUNTER )     \
    /* server behavior */                               \
    ACTION( server_eof,             STATS_COUNTER )     \
    ACTION( server_err,             STATS_COUNTER )     \
    ACTION( server_timedout,        STATS_COUNTER )     \
    ACTION( server_connections,     STATS_GAUGE   )     \
    /* data behavior */                                 \
    ACTION( requests,               STATS_COUNTER )     \
    ACTION( request_bytes,          STATS_COUNTER )     \
    ACTION( responses,              STATS_COUNTER )     \
    ACTION( response_bytes,         STATS_COUNTER )     \
    ACTION( in_queue,               STATS_GAUGE   )     \
    ACTION( in_queue_bytes,         STATS_GAUGE   )     \
    ACTION( out_queue,              STATS_GAUGE   )     \
    ACTION( out_queue_bytes,        STATS_GAUGE   )     \

#define STATS_ADDR      "localhost"
#define STATS_PORT      22222
#define STATS_INTERVAL  (30 * 1000) /* in msec */

typedef enum stats_type {
    STATS_INVALID,
    STATS_COUNTER,    /* monotonic accumulator */
    STATS_GAUGE,      /* non-monotonic accumulator */
    STATS_TIMESTAMP,  /* monotonic timestamp (in nsec) */
    STATS_SENTINEL
} stats_type_t;

struct stats_metric {
    stats_type_t  type;         /* type */
    struct string name;         /* name (ref) */
    union {
        int64_t   counter;      /* accumulating counter */
        int64_t   timestamp;    /* monotonic timestamp */
    } value;
};

struct stats_server {
    struct string name;   /* server name (ref) */
    struct array  metric; /* stats_metric[] for server codec */
};

struct stats_pool {
    struct string name;   /* pool name (ref) */
    struct array  metric; /* stats_metric[] for pool codec */
    struct array  server; /* stats_server[] */
};

struct stats_buffer {
    size_t   len;   /* buffer length */
    uint8_t  *data; /* buffer data */
    size_t   size;  /* buffer alloc size */
};

struct stats {
    uint16_t            port;           /* stats monitoring port */
    int                 interval;       /* stats aggregation interval */
    struct string       addr;           /* stats monitoring address */

    int64_t             start_ts;       /* start timestamp of nutcracker */
    struct stats_buffer buf;            /* output buffer */

    struct array        current;        /* stats_pool[] (a) */
    struct array        shadow;         /* stats_pool[] (b) */
    struct array        sum;            /* stats_pool[] (c = a + b) */

    pthread_t           tid;            /* stats aggregator thread */
    int                 sd;             /* stats descriptor */

    struct evbase       *st_evb;

    struct string       service_str;    /* service string */
    struct string       service;        /* service */
    struct string       source_str;     /* source string */
    struct string       source;         /* source */
    struct string       version_str;    /* version string */
    struct string       version;        /* version */
    struct string       uptime_str;     /* uptime string */
    struct string       timestamp_str;  /* timestamp string */

    volatile int        aggregate;      /* shadow (b) aggregate? */
    volatile int        updated;        /* current (a) updated? */
};

#define DEFINE_ACTION(_name, _type) STATS_POOL_##_name,
typedef enum stats_pool_field {
    STATS_POOL_CODEC(DEFINE_ACTION)
    STATS_POOL_NFIELD
} stats_pool_field_t;
#undef DEFINE_ACTION

#define DEFINE_ACTION(_name, _type) STATS_SERVER_##_name,
typedef enum stats_server_field {
    STATS_SERVER_CODEC(DEFINE_ACTION)
    STATS_SERVER_NFIELD
} stats_server_field_t;
#undef DEFINE_ACTION

#if defined NC_STATS && NC_STATS == 1

#define stats_pool_incr(_ctx, _pool, _name) do {                        \
    _stats_pool_incr(_ctx, _pool, STATS_POOL_##_name);                  \
} while (0)

#define stats_pool_decr(_ctx, _pool, _name) do {                        \
    _stats_pool_decr(_ctx, _pool, STATS_POOL_##_name);                  \
} while (0)

#define stats_pool_incr_by(_ctx, _pool, _name, _val) do {               \
    _stats_pool_incr_by(_ctx, _pool, STATS_POOL_##_name, _val);         \
} while (0)

#define stats_pool_decr_by(_ctx, _pool, _name, _val) do {               \
    _stats_pool_decr_by(_ctx, _pool, STATS_POOL_##_name, _val);         \
} while (0)

#define stats_server_incr(_ctx, _server, _name) do {                    \
    _stats_server_incr(_ctx, _server, STATS_SERVER_##_name);            \
} while (0)

#define stats_server_decr(_ctx, _server, _name) do {                    \
    _stats_server_decr(_ctx, _server, STATS_SERVER_##_name);            \
} while (0)

#define stats_server_incr_by(_ctx, _server, _name, _val) do {           \
    _stats_server_incr_by(_ctx, _server, STATS_SERVER_##_name, _val);   \
} while (0)

#define stats_server_decr_by(_ctx, _server, _name, _val) do {           \
    _stats_server_decr_by(_ctx, _server, STATS_SERVER_##_name, _val);   \
} while (0)

#else

#define stats_pool_incr(_ctx, _pool, _name)

#define stats_pool_decr(_ctx, _pool, _name)

#define stats_pool_incr_by(_ctx, _pool, _name, _val)

#define stats_pool_decr_by(_ctx, _pool, _name, _val)

#define stats_server_incr(_ctx, _server, _name)

#define stats_server_decr(_ctx, _server, _name)

#define stats_server_incr_by(_ctx, _server, _name, _val)

#define stats_server_decr_by(_ctx, _server, _name, _val)

#endif

#define stats_enabled   NC_STATS

void _stats_pool_incr(struct context *ctx, struct server_pool *pool, stats_pool_field_t fidx);
void _stats_pool_decr(struct context *ctx, struct server_pool *pool, stats_pool_field_t fidx);
void _stats_pool_incr_by(struct context *ctx, struct server_pool *pool, stats_pool_field_t fidx, int64_t val);
void _stats_pool_decr_by(struct context *ctx, struct server_pool *pool, stats_pool_field_t fidx, int64_t val);

void _stats_server_incr(struct context *ctx, struct server *server, stats_server_field_t fidx);
void _stats_server_decr(struct context *ctx, struct server *server, stats_server_field_t fidx);
void _stats_server_incr_by(struct context *ctx, struct server *server, stats_server_field_t fidx, int64_t val);
void _stats_server_decr_by(struct context *ctx, struct server *server, stats_server_field_t fidx, int64_t val);

struct stats *stats_create(uint16_t stats_port, int stats_interval, char *source, struct array *server_pool);
void stats_destroy(struct stats *stats);
void stats_swap(struct stats *stats);

#endif
