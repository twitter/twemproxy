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

#include <stdlib.h>
#include <unistd.h>

#include <nc_core.h>
#include <nc_server.h>
#include <nc_conf.h>
#include <nc_proxy.h>
#include <nc_introspect.h>

#define POOL_NOINDEX    (~(uint32_t)0)

void
server_ref(struct conn *conn, void *owner)
{
    struct server *server = owner;

    ASSERT(CONN_KIND_IS_SERVER(conn));
    ASSERT(conn->owner == NULL);

    conn->info = server->info;

    server->ns_conn_q++;
    TAILQ_INSERT_TAIL(&server->s_conn_q, conn, conn_tqe);

    conn->owner = owner;

    log_debug(LOG_VVERB, "ref conn %p owner %p into '%.*s", conn, server,
              server->pname.len, server->pname.data);
}

void
server_unref(struct conn *conn)
{
    struct server *server;

    ASSERT(CONN_KIND_IS_SERVER(conn));
    ASSERT(conn->owner != NULL);

    server = conn->owner;
    conn->owner = NULL;

    ASSERT(server->ns_conn_q != 0);
    server->ns_conn_q--;
    TAILQ_REMOVE(&server->s_conn_q, conn, conn_tqe);

    log_debug(LOG_VVERB, "unref conn %p owner %p from '%.*s'", conn, server,
              server->pname.len, server->pname.data);
}

int
server_timeout(struct conn *conn)
{
    struct server *server;
    struct server_pool *pool;

    ASSERT(CONN_KIND_IS_SERVER(conn));

    server = conn->owner;
    pool = server->owner;

    return pool->timeout;
}

bool
server_active(struct conn *conn)
{
    ASSERT(CONN_KIND_IS_SERVER(conn));

    if (!TAILQ_EMPTY(&conn->imsg_q)) {
        log_debug(LOG_VVERB, "s %d is active", conn->sd);
        return true;
    }

    if (!TAILQ_EMPTY(&conn->omsg_q)) {
        log_debug(LOG_VVERB, "s %d is active", conn->sd);
        return true;
    }

    if (conn->rmsg != NULL) {
        log_debug(LOG_VVERB, "s %d is active", conn->sd);
        return true;
    }

    if (conn->smsg != NULL) {
        log_debug(LOG_VVERB, "s %d is active", conn->sd);
        return true;
    }

    log_debug(LOG_VVERB, "s %d is inactive", conn->sd);

    return false;
}

static rstatus_t
server_each_set_owner(void *elem, void *data)
{
    struct server *s = elem;
    struct server_pool *sp = data;

    s->owner = sp;

    return NC_OK;
}

rstatus_t
server_init(struct array *server, struct array *conf_server,
            struct server_pool *sp)
{
    rstatus_t status;
    uint32_t nserver;

    nserver = array_n(conf_server);
    ASSERT(nserver != 0);
    ASSERT(array_n(server) == 0);

    status = array_init(server, nserver, sizeof(struct server));
    if (status != NC_OK) {
        return status;
    }

    /* transform conf server to server */
    status = array_each(conf_server, conf_server_each_transform, server);
    if (status != NC_OK) {
        server_deinit(server);
        return status;
    }
    ASSERT(array_n(server) == nserver);

    /* set server owner */
    status = array_each(server, server_each_set_owner, sp);
    if (status != NC_OK) {
        server_deinit(server);
        return status;
    }

    log_debug(LOG_DEBUG, "init %"PRIu32" servers in pool %"PRIu32" '%.*s'",
              nserver, sp->idx, sp->name.len, sp->name.data);

    return NC_OK;
}

void
server_deinit(struct array *server)
{
    uint32_t i, nserver;

    for (i = 0, nserver = array_n(server); i < nserver; i++) {
        struct server *s;

        s = array_pop(server);
        ASSERT(TAILQ_EMPTY(&s->s_conn_q) && s->ns_conn_q == 0);

        string_deinit(&s->pname);
        string_deinit(&s->name);
    }
    array_deinit(server);
}

struct conn *
server_conn(struct server *server)
{
    struct server_pool *pool;
    struct conn *conn;

    pool = server->owner;

    /*
     * FIXME: handle multiple server connections per server and do load
     * balancing on it. Support multiple algorithms for
     * 'server_connections:' > 0 key
     */

    if (server->ns_conn_q < pool->server_connections) {
        return conn_get(server, pool->redis
            ? NC_CONN_SERVER_REDIS : NC_CONN_SERVER_MEMCACHE);
    }
    ASSERT(server->ns_conn_q == pool->server_connections);

    /*
     * Pick a server connection from the head of the queue and insert
     * it back into the tail of queue to maintain the lru order
     */
    conn = TAILQ_FIRST(&server->s_conn_q);
    ASSERT(CONN_KIND_IS_SERVER(conn));

    TAILQ_REMOVE(&server->s_conn_q, conn, conn_tqe);
    TAILQ_INSERT_TAIL(&server->s_conn_q, conn, conn_tqe);

    return conn;
}

static rstatus_t
server_each_preconnect(void *elem, void *data)
{
    rstatus_t status;
    struct server *server;
    struct server_pool *pool;
    struct conn *conn;

    server = elem;
    pool = server->owner;

    conn = server_conn(server);
    if (conn == NULL) {
        return NC_ENOMEM;
    }

    status = server_connect(pool->ctx, server, conn);
    if (status != NC_OK) {
        log_warn("connect to server '%.*s' failed, ignored: %s",
                 server->pname.len, server->pname.data, strerror(errno));
        server_close(pool->ctx, conn);
    }

    return NC_OK;
}

static rstatus_t
server_each_disconnect(void *elem, void *data)
{
    struct server *server;
    struct server_pool *pool;

    server = elem;
    pool = server->owner;

    while (!TAILQ_EMPTY(&server->s_conn_q)) {
        struct conn *conn;

        ASSERT(server->ns_conn_q > 0);

        conn = TAILQ_FIRST(&server->s_conn_q);
        conn->close(pool->ctx, conn);
    }

    return NC_OK;
}

static void
server_failure(struct context *ctx, struct server *server)
{
    struct server_pool *pool = server->owner;
    int64_t now, next;
    rstatus_t status;

    if (!pool->auto_eject_hosts) {
        return;
    }

    server->failure_count++;

    log_debug(LOG_VERB, "server '%.*s' failure count %"PRIu32" limit %"PRIu32,
              server->pname.len, server->pname.data, server->failure_count,
              pool->server_failure_limit);

    if (server->failure_count < pool->server_failure_limit) {
        return;
    }

    now = nc_usec_now();
    if (now < 0) {
        return;
    }

    stats_server_set_ts(ctx, server, server_ejected_at, now);

    next = now + pool->server_retry_timeout;

    log_debug(LOG_INFO, "update pool %"PRIu32" '%.*s' to delete server '%.*s' "
              "for next %"PRIu32" secs", pool->idx, pool->name.len,
              pool->name.data, server->pname.len, server->pname.data,
              pool->server_retry_timeout / 1000 / 1000);

    stats_pool_incr(ctx, pool, server_ejects);

    server->failure_count = 0;
    server->next_retry = next;

    status = server_pool_run(pool);
    if (status != NC_OK) {
        log_error("updating pool %"PRIu32" '%.*s' failed: %s", pool->idx,
                  pool->name.len, pool->name.data, strerror(errno));
    }
}

static void
server_close_stats(struct context *ctx, struct server *server, err_t err,
                   unsigned eof, unsigned connected)
{
    if (connected) {
        stats_server_decr(ctx, server, server_connections);
    }

    if (eof) {
        stats_server_incr(ctx, server, server_eof);
        return;
    }

    switch (err) {
    case ETIMEDOUT:
        stats_server_incr(ctx, server, server_timedout);
        break;
    case EPIPE:
    case ECONNRESET:
    case ECONNABORTED:
    case ECONNREFUSED:
    case ENOTCONN:
    case ENETDOWN:
    case ENETUNREACH:
    case EHOSTDOWN:
    case EHOSTUNREACH:
    default:
        stats_server_incr(ctx, server, server_err);
        break;
    }
}

void
server_close(struct context *ctx, struct conn *conn)
{
    rstatus_t status;
    struct msg *msg, *nmsg; /* current and next message */
    struct conn *c_conn;    /* peer client connection */

    ASSERT(CONN_KIND_IS_SERVER(conn));

    server_close_stats(ctx, conn->owner, conn->err, conn->eof,
                       conn->connected);

    conn->connected = false;

    if (conn->sd < 0) {
        server_failure(ctx, conn->owner);
        conn->unref(conn);
        conn_put(conn);
        return;
    }

    for (msg = TAILQ_FIRST(&conn->imsg_q); msg != NULL; msg = nmsg) {
        nmsg = TAILQ_NEXT(msg, s_tqe);

        /* dequeue the message (request) from server inq */
        conn->dequeue_inq(ctx, conn, msg);

        /*
         * Don't send any error response, if
         * 1. request is tagged as noreply or,
         * 2. client has already closed its connection
         */
        if (msg->swallow || msg->noreply) {
            log_debug(LOG_INFO, "close %s %d swallow req %"PRIu64" len %"PRIu32
                      " type %d", CONN_KIND_AS_STRING(conn), conn->sd,
                      msg->id, msg->mlen, msg->type);
            req_put(msg);
        } else {
            c_conn = msg->owner;
            ASSERT(CONN_KIND_IS_CLIENT(c_conn));

            msg->done = 1;
            msg->error = 1;
            msg->err = conn->err;

            if (msg->frag_owner != NULL) {
                msg->frag_owner->nfrag_done++;
            }

            if (req_done(c_conn, TAILQ_FIRST(&c_conn->omsg_q))) {
                event_add_out(ctx->evb, msg->owner);
            }

            log_debug(LOG_INFO, "close %s %d schedule error for req %"PRIu64" "
                      "len %"PRIu32" type %d from %s %d%c %s",
                      CONN_KIND_AS_STRING(conn), conn->sd, msg->id,
                      msg->mlen, msg->type,
                      CONN_KIND_AS_STRING(c_conn), c_conn->sd,
                      conn->err ? ':' : ' ',
                      conn->err ? strerror(conn->err): " ");
        }
    }
    ASSERT(TAILQ_EMPTY(&conn->imsg_q));

    for (msg = TAILQ_FIRST(&conn->omsg_q); msg != NULL; msg = nmsg) {
        nmsg = TAILQ_NEXT(msg, s_tqe);

        /* dequeue the message (request) from server outq */
        conn->dequeue_outq(ctx, conn, msg);

        if (msg->swallow) {
            log_debug(LOG_INFO, "close %s %d swallow req %"PRIu64" len %"PRIu32
                      " type %d", CONN_KIND_AS_STRING(conn), conn->sd,
                      msg->id, msg->mlen, msg->type);
            req_put(msg);
        } else {
            c_conn = msg->owner;
            ASSERT(CONN_KIND_IS_CLIENT(c_conn));

            msg->done = 1;
            msg->error = 1;
            msg->err = conn->err;
            if (msg->frag_owner != NULL) {
                msg->frag_owner->nfrag_done++;
            }

            if (req_done(c_conn, TAILQ_FIRST(&c_conn->omsg_q))) {
                event_add_out(ctx->evb, msg->owner);
            }

            log_debug(LOG_INFO, "close %s %d schedule error for req %"PRIu64" "
                      "len %"PRIu32" type %d from %s %d%c %s",
                      CONN_KIND_AS_STRING(conn), conn->sd, msg->id,
                      msg->mlen, msg->type,
                      CONN_KIND_AS_STRING(c_conn), c_conn->sd,
                      conn->err ? ':' : ' ',
                      conn->err ? strerror(conn->err): " ");
        }
    }
    ASSERT(TAILQ_EMPTY(&conn->omsg_q));

    msg = conn->rmsg;
    if (msg != NULL) {
        conn->rmsg = NULL;

        ASSERT(!msg->request);
        ASSERT(msg->peer == NULL);

        rsp_put(msg);

        log_debug(LOG_INFO, "close %s %d discarding rsp %"PRIu64" len %"PRIu32" "
                  "in error", CONN_KIND_AS_STRING(conn), conn->sd,
                  msg->id, msg->mlen);
    }

    ASSERT(conn->smsg == NULL);

    server_failure(ctx, conn->owner);

    conn->unref(conn);

    status = close(conn->sd);
    if (status < 0) {
        log_error("close %s %d failed, ignored: %s",
                  CONN_KIND_AS_STRING(conn), conn->sd, strerror(errno));
    }
    conn->sd = -1;

    conn_put(conn);
}

rstatus_t
server_connect(struct context *ctx, struct server *server, struct conn *conn)
{
    rstatus_t status;

    ASSERT(CONN_KIND_IS_SERVER(conn));

    if (conn->sd > 0) {
        /* already connected on server connection */
        return NC_OK;
    }

    log_debug(LOG_VVERB, "connect to server '%.*s'", server->pname.len,
              server->pname.data);

    conn->sd = socket(conn->info.family, SOCK_STREAM, 0);
    if (conn->sd < 0) {
        log_error("socket for server '%.*s' failed: %s", server->pname.len,
                  server->pname.data, strerror(errno));
        status = NC_ERROR;
        goto error;
    }

    status = nc_set_nonblocking(conn->sd);
    if (status != NC_OK) {
        log_error("set nonblock on %s %d for server '%.*s' failed: %s",
                  CONN_KIND_AS_STRING(conn), conn->sd,
                  server->pname.len, server->pname.data, strerror(errno));
        goto error;
    }

    if (server->pname.data[0] != '/') {
        status = nc_set_tcpnodelay(conn->sd);
        if (status != NC_OK) {
            log_warn("set tcpnodelay on %s %d for server '%.*s' failed, ignored: %s",
                     CONN_KIND_AS_STRING(conn), conn->sd,
                     server->pname.len, server->pname.data,
                     strerror(errno));
        }
    }

    status = event_add_conn(ctx->evb, conn);
    if (status != NC_OK) {
        log_error("event add conn %s %d for server '%.*s' failed: %s",
                  CONN_KIND_AS_STRING(conn), conn->sd,
                  server->pname.len, server->pname.data,
                  strerror(errno));
        goto error;
    }

    ASSERT(!conn->connecting && !conn->connected);

    status = connect(conn->sd, (struct sockaddr *)&conn->info.addr,
                               conn->info.addrlen);
    if (status != NC_OK) {
        if (errno == EINPROGRESS) {
            conn->connecting = 1;
            log_debug(LOG_DEBUG, "connecting on %s %d to server '%.*s'",
                      CONN_KIND_AS_STRING(conn), conn->sd,
                      server->pname.len, server->pname.data);
            return NC_OK;
        }

        log_error("connect on %s %d to server '%.*s' (%s) failed: %s",
                  CONN_KIND_AS_STRING(conn), conn->sd,
                  server->pname.len, server->pname.data,
                  conn_unresolve_descriptive(conn), strerror(errno));

        goto error;
    }

    ASSERT(!conn->connecting);
    conn->connected = 1;
    log_debug(LOG_INFO, "connected on %s %d to server '%.*s'",
              CONN_KIND_AS_STRING(conn), conn->sd,
              server->pname.len, server->pname.data);

    return NC_OK;

error:
    conn->err = errno;
    return status;
}

void
server_connected(struct context *ctx, struct conn *conn)
{
    struct server *server = conn->owner;

    ASSERT(CONN_KIND_IS_SERVER(conn));
    ASSERT(conn->connecting && !conn->connected);

    stats_server_incr(ctx, server, server_connections);

    conn->connecting = 0;
    conn->connected = 1;

    conn->post_connect(ctx, conn, server);

    log_debug(LOG_INFO, "connected on %s %d to server '%.*s'",
              CONN_KIND_AS_STRING(conn), conn->sd,
              server->pname.len, server->pname.data);
}

void
server_ok(struct context *ctx, struct conn *conn)
{
    struct server *server = conn->owner;

    ASSERT(CONN_KIND_IS_SERVER(conn));
    ASSERT(conn->connected);

    if (server->failure_count != 0) {
        log_debug(LOG_VERB, "reset server '%.*s' failure count from %"PRIu32
                  " to 0", server->pname.len, server->pname.data,
                  server->failure_count);
        server->failure_count = 0;
        server->next_retry = 0LL;
    }
}

static rstatus_t
server_pool_update(struct server_pool *pool)
{
    rstatus_t status;
    int64_t now;
    uint32_t pnlive_server; /* prev # live server */

    if (!pool->auto_eject_hosts) {
        return NC_OK;
    }

    if (pool->next_rebuild == 0LL) {
        return NC_OK;
    }

    now = nc_usec_now();
    if (now < 0) {
        return NC_ERROR;
    }

    if (now <= pool->next_rebuild) {
        if (pool->nlive_server == 0) {
            errno = ECONNREFUSED;
            return NC_ERROR;
        }
        return NC_OK;
    }

    pnlive_server = pool->nlive_server;

    status = server_pool_run(pool);
    if (status != NC_OK) {
        log_error("updating pool %"PRIu32" with dist %d failed: %s", pool->idx,
                  pool->dist_type, strerror(errno));
        return status;
    }

    log_debug(LOG_INFO, "update pool %"PRIu32" '%.*s' to add %"PRIu32" servers",
              pool->idx, pool->name.len, pool->name.data,
              pool->nlive_server - pnlive_server);


    return NC_OK;
}

static uint32_t
server_pool_hash(struct server_pool *pool, uint8_t *key, uint32_t keylen)
{
    ASSERT(array_n(&pool->server) != 0);
    ASSERT(key != NULL);

    if (array_n(&pool->server) == 1) {
        return 0;
    }

    if (keylen == 0) {
        return 0;
    }

    return pool->key_hash((char *)key, keylen);
}

uint32_t
server_pool_idx(struct server_pool *pool, uint8_t *key, uint32_t keylen)
{
    uint32_t hash, idx;

    ASSERT(array_n(&pool->server) != 0);
    ASSERT(key != NULL);

    /*
     * If hash_tag: is configured for this server pool, we use the part of
     * the key within the hash tag as an input to the distributor. Otherwise
     * we use the full key
     */
    if (!string_empty(&pool->hash_tag)) {
        struct string *tag = &pool->hash_tag;
        uint8_t *tag_start, *tag_end;

        tag_start = nc_strchr(key, key + keylen, tag->data[0]);
        if (tag_start != NULL) {
            tag_end = nc_strchr(tag_start + 1, key + keylen, tag->data[1]);
            if ((tag_end != NULL) && (tag_end - tag_start > 1)) {
                key = tag_start + 1;
                keylen = (uint32_t)(tag_end - key);
            }
        }
    }

    switch (pool->dist_type) {
    case DIST_KETAMA:
        hash = server_pool_hash(pool, key, keylen);
        idx = ketama_dispatch(pool->continuum, pool->ncontinuum, hash);
        break;

    case DIST_MODULA:
        hash = server_pool_hash(pool, key, keylen);
        idx = modula_dispatch(pool->continuum, pool->ncontinuum, hash);
        break;

    case DIST_RANDOM:
        idx = random_dispatch(pool->continuum, pool->ncontinuum, 0);
        break;

    default:
        NOT_REACHED();
        return 0;
    }
    ASSERT(idx < array_n(&pool->server));
    return idx;
}

static struct server *
server_pool_server(struct server_pool *pool, uint8_t *key, uint32_t keylen)
{
    struct server *server;
    uint32_t idx;

    idx = server_pool_idx(pool, key, keylen);
    server = array_get(&pool->server, idx);

    log_debug(LOG_VERB, "key '%.*s' on dist %d maps to server '%.*s'", keylen,
              key, pool->dist_type, server->pname.len, server->pname.data);

    return server;
}

struct conn *
server_pool_conn(struct context *ctx, struct server_pool *pool, uint8_t *key,
                 uint32_t keylen)
{
    rstatus_t status;
    struct server *server;
    struct conn *conn;

    status = server_pool_update(pool);
    if (status != NC_OK) {
        return NULL;
    }

    /* from a given {key, keylen} pick a server from pool */
    server = server_pool_server(pool, key, keylen);
    if (server == NULL) {
        return NULL;
    }

    /* pick a connection to a given server */
    conn = server_conn(server);
    if (conn == NULL) {
        return NULL;
    }

    status = server_connect(ctx, server, conn);
    if (status != NC_OK) {
        server_close(ctx, conn);
        return NULL;
    }

    return conn;
}

static rstatus_t
server_pool_preconnect(struct server_pool *sp, void *data)
{
    rstatus_t status;

    if (!sp->preconnect) {
        return NC_OK;
    }

    status = array_each(&sp->server, server_each_preconnect, NULL);
    if (status != NC_OK) {
        return status;
    }

    return NC_OK;
}

rstatus_t
server_pool_each(struct server_pools *server_pools, pool_each_t func, void *key)
{
    rstatus_t status;
    struct server_pool *pool, *tmpool;

    TAILQ_FOREACH_SAFE(pool, server_pools, pool_tqe, tmpool) {
        status = func(pool, key);
        if (status != NC_OK) {
            return status;
        }
    }

    return NC_OK;
}

rstatus_t
server_pools_preconnect(struct context *ctx)
{
    return server_pool_each(&ctx->pools, server_pool_preconnect, NULL);
}

static rstatus_t
server_pool_disconnect(struct server_pool *sp, void *data)
{
    return array_each(&sp->server, server_each_disconnect, NULL);
}

void
server_pools_disconnect(struct server_pools *server_pools)
{
    server_pool_each(server_pools, server_pool_disconnect, NULL);
}

rstatus_t
server_pool_run(struct server_pool *pool)
{
    ASSERT(array_n(&pool->server) != 0);

    switch (pool->dist_type) {
    case DIST_KETAMA:
        return ketama_update(pool);

    case DIST_MODULA:
        return modula_update(pool);

    case DIST_RANDOM:
        return random_update(pool);

    default:
        NOT_REACHED();
        return NC_ERROR;
    }

    return NC_OK;
}

rstatus_t
server_pools_init(struct server_pools *server_pools, struct array *conf_pool,
                 struct context *ctx)
{
    rstatus_t status;
    uint32_t npool;
    uint32_t n_server_pools;
    struct server_pool *pool;

    npool = array_n(conf_pool);
    ASSERT(npool != 0);

    /* transform conf pool to server pool */
    status = array_each(conf_pool, conf_pool_each_transform, server_pools);
    if (status != NC_OK) {
        server_pools_deinit(server_pools);
        return status;
    }

    /*
     * Do a post-initialization work on all server pools.
     */
    n_server_pools = 0;
    TAILQ_FOREACH(pool, server_pools, pool_tqe) {
        pool->ctx = ctx;  /* Set owner */
        pool->idx = n_server_pools++;
        /* compute max server connections */
        ctx->max_nsconn += pool->server_connections * array_n(&pool->server);
        ctx->max_nsconn += 1; /* pool listening socket */
        /* update server pool continuum */
        server_pool_run(pool);
    }
    ASSERT(npool == n_server_pools);

    log_debug(LOG_DEBUG, "init %"PRIu32" pools", n_server_pools);

    return NC_OK;
}

struct server_pool *
server_pool_new()
{
    struct server_pool *pool;
    pool = nc_calloc(1, sizeof(struct server_pool));
    return pool;
}

void
server_pool_free(struct server_pool *pool)
{
    string_deinit(&pool->name);
    string_deinit(&pool->addrstr);
    string_deinit(&pool->hash_tag);
    string_deinit(&pool->redis_auth);
    nc_free(pool);  /* FIXME: memory leaks here. */
}

static void
server_pool_move_client_connections(struct server_pool *from, struct server_pool *to) {
    struct conn *conn, *tmp_conn;

    TAILQ_INIT(&to->c_conn_q);
    to->nc_conn_q = 0;

    TAILQ_FOREACH_SAFE(conn, &from->c_conn_q, conn_tqe, tmp_conn) {
        TAILQ_REMOVE(&from->c_conn_q, conn, conn_tqe);
        TAILQ_INSERT_TAIL(&to->c_conn_q, conn, conn_tqe);
        conn->owner = to;
        to->nc_conn_q++;
    }

    ASSERT(to->nc_conn_q == from->nc_conn_q);

    from->nc_conn_q = 0;
    TAILQ_INIT(&from->c_conn_q);
}

static rstatus_t
server_pool_deinit(struct server_pool *pool, void *data) {

    ASSERT(pool->p_conn == NULL);
    ASSERT(TAILQ_EMPTY(&pool->c_conn_q) && pool->nc_conn_q == 0);

    if(pool->pool_counterpart) {
        pool->pool_counterpart->pool_counterpart = 0;
        pool->pool_counterpart = 0;
    }

    if (pool->continuum != NULL) {
        nc_free(pool->continuum);
        pool->ncontinuum = 0;
        pool->nserver_continuum = 0;
        pool->nlive_server = 0;
    }

    server_pool_disconnect(pool, NULL);
    server_deinit(&pool->server);

    log_debug(LOG_DEBUG, "deinit pool %"PRIu32" '%.*s'", pool->idx,
              pool->name.len, pool->name.data);

    if(pool->ctx) {
        TAILQ_REMOVE(&pool->ctx->pools, pool, pool_tqe);
    }
    server_pool_free(pool);

    return NC_OK;
}

void
server_pools_deinit(struct server_pools *server_pools)
{
    server_pool_each(server_pools, server_pool_deinit, NULL);

    log_debug(LOG_DEBUG, "deinit pools");
}

uint32_t
server_pools_n(struct server_pools *server_pools) {
    struct server_pool *pool;
    uint32_t npool = 0;

    TAILQ_FOREACH(pool, server_pools, pool_tqe) {
        npool++;
    }

    return npool;
}


/*
 * Convert between each (flat) and hierarchical fold.
 */
struct morphism_and_accumulator {
    nc_morphism_f morphism;
    void *accumulator;
};

static rstatus_t server_pool_each_to_fold(void *elem, void *data) {
    struct morphism_and_accumulator *mac = data;
    struct server *server = elem;
    struct conn *conn;
    uint32_t count;

    mac->accumulator = mac->morphism(NC_ELEMENT_IS_SERVER,
                                     server, mac->accumulator);

    count = 0;
    /* Iterate over server connections going to a single server. */
    TAILQ_FOREACH(conn, &server->s_conn_q, conn_tqe) {
        mac->accumulator = mac->morphism(NC_ELEMENT_IS_CONNECTION,
                                         conn, mac->accumulator);
        count++;
    }
    ASSERT(server->ns_conn_q == count);

    return NC_OK;
}

static void *
server_pool_fold(struct server_pool *pool, nc_morphism_f f, void *acc) {
    struct morphism_and_accumulator mac;
    struct conn *conn;
    rstatus_t status;

    acc = f(NC_ELEMENT_IS_POOL, pool, acc);

    if(pool->p_conn)
        acc = f(NC_ELEMENT_IS_CONNECTION, pool->p_conn, acc);

    /* Iterate over client connections accessing a single pool proxy. */
    TAILQ_FOREACH(conn, &pool->c_conn_q, conn_tqe) {
        acc = f(NC_ELEMENT_IS_CONNECTION, conn, acc);
    }

    mac.morphism = f;
    mac.accumulator = acc;

    status = array_each(&pool->server, server_pool_each_to_fold, &mac);
    ASSERT(status == NC_OK);

    return mac.accumulator;
}

void *
server_pools_fold(struct server_pools *server_pools, nc_morphism_f f, void *acc) {
    struct server_pool *pool;

    TAILQ_FOREACH(pool, server_pools, pool_tqe) {
        acc = server_pool_fold(pool, f, acc);
    }

    return acc;
}

/*
 * Set a given reload state to all pools.
 */
static void
server_pools_set_reload_state(struct server_pools *server_pools, enum server_pools_reload_state state) {
    struct server_pool *pool;

    TAILQ_FOREACH(pool, server_pools, pool_tqe) {
        pool->reload_state = state;
    }
}

/*
 * Check that all reload states equal to the given state.
 */
static bool
server_pools_check_reload_state(struct server_pools *pools, enum server_pools_reload_state state) {
    struct server_pool *pool;

    TAILQ_FOREACH(pool, pools, pool_tqe) {
        if(pool->reload_state != state)
            return false;
    }
    return true;
}

/*
 * Pause ingress stream of data from clients, and accepting new ones.
 */
static void
server_pool_pause_incoming_client_traffic(struct server_pool *pool) {
    log_debug(LOG_DEBUG, "Pausing client connections for pool '%.*s' (%s)",
              pool->name.len, pool->name.data,
              nc_unresolve(&pool->p_conn->info));

    /* Pause proxy connection (not accepting new clients) */
    event_del_in(pool->ctx->evb, pool->p_conn);

    /* Pause client connections */
    struct conn *conn;
    TAILQ_FOREACH(conn, &pool->c_conn_q, conn_tqe) {
        event_del_in(pool->ctx->evb, conn);
    }

}

static void
server_pool_resume_incoming_client_traffic(struct server_pool *pool) {
    log_debug(LOG_DEBUG, "Resume client connections for pool '%.*s' (%s)",
              pool->name.len, pool->name.data,
              nc_unresolve(&pool->p_conn->info));

    /* Resume proxy connection (accepting new clients) */
    event_add_in(pool->ctx->evb, pool->p_conn);

    /* Resume client connections */
    struct conn *conn;
    TAILQ_FOREACH(conn, &pool->c_conn_q, conn_tqe) {
        event_add_in(pool->ctx->evb, conn);
    }
}

struct last_not_drained {
    struct conn *conn_not_drained;
    int num_not_drained;
};

static void *
connection_is_drained(enum nc_morph_elem_type etype, void *elem, void *acc0) {

    if(etype == NC_ELEMENT_IS_CONNECTION) {
        struct conn *conn = elem;
        struct last_not_drained *nd = acc0;

        if((conn->rmsg == NULL
                || msg_empty(conn->rmsg))
            && conn->smsg == NULL
            && TAILQ_EMPTY(&conn->imsg_q)
            && (TAILQ_EMPTY(&conn->omsg_q)
                || CONN_KIND_IS_CLIENT(conn))
        ) {
            /* Connection is effectively drained. */
        } else {
            nd->conn_not_drained = conn;
            nd->num_not_drained++;
        }
    }

    return acc0;
}

/*
 * We decide that the pool is drained when there are no outstanding
 * unprocessed messages in the client and server connections.
 */
static bool
server_pool_drained(struct server_pool *pool) {
    struct last_not_drained nd = { 0 };

    server_pool_fold(pool, connection_is_drained, &nd);

    if(nd.conn_not_drained) {
        log_debug(LOG_DEBUG, "something (e.g. %s %s) is still not drained (%d)",
            CONN_KIND_AS_STRING(nd.conn_not_drained),
            conn_unresolve_descriptive(nd.conn_not_drained),
            nd.num_not_drained);
    }

    if(nd.conn_not_drained)
        return false;
    else
        return true;
}

/*
 * For each reload state there's some things to do before we consider
 * reloading safely initiated.
 */
static rstatus_t
server_pools_kick_state_machine(struct server_pools *pools)
{
    struct server_pool *pool, *tpool;
    rstatus_t rstatus;

    TAILQ_FOREACH_SAFE(pool, pools, pool_tqe, tpool) {
        switch(pool->reload_state) {
        case RSTATE_OLD_AND_ACTIVE:
        case RSTATE_NEW_WAIT_FOR_OLD:
            break;
        case RSTATE_NEW:
            rstatus = proxy_each_init(pool, 0);
            if(rstatus != NC_OK) {
                return rstatus;
            }
            pool->reload_state = RSTATE_OLD_AND_ACTIVE;
            break;
        case RSTATE_OLD_TO_SHUTDOWN:
            if(!pool->pool_counterpart) {
                /*
                 * This pool is not needed anymore. Shut down immediately.
                 * NOTE: since we're are going to be able to undo
                 * the changes, the pools are organized so the new pools
                 * are placed at the beginning of the TAILQ. This way,
                 * all errors to initialize the new pools can materialize
                 * before we get to the old pools and start deiniting them.
                 */
                ASSERT(pool->p_conn);
                proxy_each_deinit(pool, 0);
                pool->p_conn = 0;
                server_pool_deinit(pool, 0);
                break;
            } else {
                ASSERT(pool->p_conn);
                server_pool_pause_incoming_client_traffic(pool);
                pool->reload_state = RSTATE_OLD_DRAINING;
            }
            break;
        case RSTATE_OLD_DRAINING:
            if(server_pool_drained(pool)) {
                struct server_pool *npool = pool->pool_counterpart;
                ASSERT(pool->pool_counterpart);
                ASSERT(npool->pool_counterpart == pool);
                ASSERT(npool->p_conn == false);

                npool->pool_counterpart = 0;
                pool->pool_counterpart = 0;

                /* Move proxy connection */
                npool->p_conn = pool->p_conn;
                npool->p_conn->owner = npool;
                pool->p_conn = NULL;
                /* Move client connections over as well */
                server_pool_move_client_connections(pool, npool);
                server_pool_resume_incoming_client_traffic(npool);
                npool->reload_state = RSTATE_OLD_AND_ACTIVE;
                server_pool_run(npool);

                /* Remove the old pool*/
                server_pool_deinit(pool, 0);
            }
            break;
        }
    }

    return NC_OK;
}

static void
server_pools_undo_partial_reload(struct server_pools *pools) {
    struct server_pool *pool, *tpool;

    log_error("Can't reload configuration, reverting back pool changes");

    TAILQ_FOREACH_SAFE(pool, pools, pool_tqe, tpool) {
        switch(pool->reload_state) {
        case RSTATE_OLD_AND_ACTIVE:
            break;
        case RSTATE_OLD_TO_SHUTDOWN:
            pool->reload_state = RSTATE_OLD_AND_ACTIVE;
            pool->pool_counterpart = 0;
            break;
        case RSTATE_OLD_DRAINING:
            /* Unpause getting the data from clients */
            server_pool_resume_incoming_client_traffic(pool);
            pool->reload_state = RSTATE_OLD_AND_ACTIVE;
            pool->pool_counterpart = 0;
            break;
        case RSTATE_NEW:
            server_pool_deinit(pool, 0);
            break;
        case RSTATE_NEW_WAIT_FOR_OLD:
            server_pool_deinit(pool, 0);
            break;
        }
    }

    /* Just checking that the state is back to normal */
    TAILQ_FOREACH(pool, pools, pool_tqe) {
        ASSERT(pool->reload_state == RSTATE_OLD_AND_ACTIVE);
    }
}

rstatus_t
server_pools_kick_replacement(struct server_pools *old_pools, struct server_pools *new_pools)
{
    struct server_pool *npool, *tmp_npool;
    struct server_pool *opool;

    ASSERT(server_pools_check_reload_state(old_pools, RSTATE_OLD_AND_ACTIVE));
    ASSERT(server_pools_n(new_pools) != 0);

    server_pools_set_reload_state(old_pools, RSTATE_OLD_AND_ACTIVE); server_pools_set_reload_state(new_pools, RSTATE_NEW);

    /*
     * Establish correspondence between old and new pools.
     */
    TAILQ_FOREACH(opool, old_pools, pool_tqe) {
        TAILQ_FOREACH_SAFE(npool, new_pools, pool_tqe, tmp_npool) {
            if (npool->reload_state == RSTATE_NEW && string_compare(&opool->name, &npool->name) == 0) {
                npool->reload_state = RSTATE_NEW_WAIT_FOR_OLD;
                opool->reload_state = RSTATE_OLD_TO_SHUTDOWN;
                npool->pool_counterpart = opool;
                opool->pool_counterpart = npool;
            }
        }
    }

    /*
     * Initiate removal of unused pools.
     */
    TAILQ_FOREACH(opool, old_pools, pool_tqe) {
        if(opool->reload_state == RSTATE_OLD_AND_ACTIVE) {
            opool->reload_state = RSTATE_OLD_TO_SHUTDOWN;
            opool->pool_counterpart = 0;
        }
    }

    /*
     * Move new pools into the new ones.
     * NOTE: since we're are going to be able to undo
     * the changes, the pools are organized so the new pools
     * are placed at the beginning of the TAILQ. This way,
     * all errors to initialize the new pools can materialize
     * before we get to the old pools and start deiniting them.
     * NOTE: however, the index (pool->idx) of the new pools
     * should correspond to their natural order in the list.
     * So we move the items one by one from the tail of the new pool
     * into the beginning of the target pool.
     */
    TAILQ_FOREACH_REVERSE_SAFE(npool, new_pools, server_pools, pool_tqe, tmp_npool) {
        TAILQ_REMOVE(new_pools, npool, pool_tqe);
        TAILQ_INSERT_HEAD(old_pools, npool, pool_tqe);
    }
    new_pools = 0;  /* Do not touch this any more */

    /*
     * Kick the state machine for each of the new and old pools.
     */
    if(server_pools_kick_state_machine(old_pools) != NC_OK) {
        server_pools_undo_partial_reload(old_pools);
        return NC_ERROR;
    } else {
        /*
         * Finally change the state of the new pools if everything was
         * properly initialized (and all new proxy connections was properly
         * bound to ip addresses).
         */
        TAILQ_FOREACH(opool, old_pools, pool_tqe) {
            if(opool->reload_state == RSTATE_NEW)
                opool->reload_state = RSTATE_OLD_AND_ACTIVE;
        }
    }

    return NC_OK;
}

/*
 * Attempt to complete the code reload (pool replacement) process.
 */
bool
server_pools_finish_replacement(struct server_pools *pools)
{

    log_debug(LOG_DEBUG, "replacement completion test invoked");

    rstatus_t rstatus;
    rstatus = server_pools_kick_state_machine(pools);
    ASSERT(rstatus == NC_OK);

    bool finished;
    finished = server_pools_check_reload_state(pools, RSTATE_OLD_AND_ACTIVE);
    log_debug(LOG_DEBUG, "replacement %s",
        finished ? "is finished" : "is still in progress");
    return finished;
}


void
server_pools_log(int level, const char *prefix, struct server_pools *pools)
{
    log_debug(LOG_NOTICE, "%s", prefix);
    log_runtime_objects(LOG_NOTICE, TAILQ_FIRST(pools)->ctx, pools,
        FRO_POOLS | FRO_SERVERS | FRO_SERVER_CONNS
        | (level >= LOG_DEBUG ? FRO_CLIENT_CONNS : 0)
        | (level >= LOG_DEBUG ? FRO_DETAIL_VERBOSE : 0));
}

