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

#include <sys/stat.h>
#include <sys/un.h>

#include <nc_core.h>
#include <nc_server.h>
#include <nc_proxy.h>

void
proxy_ref(struct conn *conn, void *owner)
{
    struct server_pool *pool = owner;

    ASSERT(CONN_KIND_IS_PROXY(conn));
    ASSERT(conn->owner == NULL);

    conn->info = pool->info;

    pool->p_conn = conn;

    /* owner of the proxy connection is the server pool */
    conn->owner = owner;

    log_debug(LOG_VVERB, "ref conn %p owner %p into pool %"PRIu32"", conn,
              pool, pool->idx);
}

void
proxy_unref(struct conn *conn)
{
    struct server_pool *pool;

    ASSERT(CONN_KIND_IS_PROXY(conn));
    ASSERT(conn->owner != NULL);

    pool = conn->owner;
    conn->owner = NULL;

    pool->p_conn = NULL;

    log_debug(LOG_VVERB, "unref conn %p owner %p from pool %"PRIu32"", conn,
              pool, pool->idx);
}

void
proxy_close(struct context *ctx, struct conn *conn)
{
    rstatus_t status;

    ASSERT(CONN_KIND_IS_PROXY(conn));

    if (conn->sd < 0) {
        conn->unref(conn);
        conn_put(conn);
        return;
    }

    ASSERT(conn->rmsg == NULL);
    ASSERT(conn->smsg == NULL);
    ASSERT(TAILQ_EMPTY(&conn->imsg_q));
    ASSERT(TAILQ_EMPTY(&conn->omsg_q));

    conn->unref(conn);

    status = close(conn->sd);
    if (status < 0) {
        log_error("close %s %d failed, ignored: %s",
                  CONN_KIND_AS_STRING(conn), conn->sd, strerror(errno));
    }
    conn->sd = -1;

    conn_put(conn);
}

static rstatus_t
proxy_reuse(struct conn *p)
{
    rstatus_t status;
    struct sockaddr_un *un;

    switch (p->info.family) {
    case AF_INET:
    case AF_INET6:
        status = nc_set_reuseaddr(p->sd);
        break;

    case AF_UNIX:
        /*
         * bind() will fail if the pathname already exist. So, we call unlink()
         * to delete the pathname, in case it already exists. If it does not
         * exist, unlink() returns error, which we ignore
         */
        un = &p->info.addr.un;
        unlink(un->sun_path);
        status = NC_OK;
        break;

    default:
        NOT_REACHED();
        status = NC_ERROR;
    }

    return status;
}

static rstatus_t
proxy_listen(struct context *ctx, struct conn *p)
{
    rstatus_t status;
    struct server_pool *pool = p->owner;

    ASSERT(CONN_KIND_IS_PROXY(p));

    p->sd = socket(p->info.family, SOCK_STREAM, 0);
    if (p->sd < 0) {
        log_error("socket failed: %s", strerror(errno));
        return NC_ERROR;
    }

    status = proxy_reuse(p);
    if (status < 0) {
        log_error("reuse of addr '%.*s' for listening on %s %d failed: %s",
                  pool->addrstr.len, pool->addrstr.data,
                  CONN_KIND_AS_STRING(p), p->sd, strerror(errno));
        return NC_ERROR;
    }

    status = bind(p->sd, (struct sockaddr *)&p->info.addr, p->info.addrlen);
    if (status < 0) {
        log_error("bind on %s %d to addr '%.*s' failed: %s",
                  CONN_KIND_AS_STRING(p), p->sd,
                  pool->addrstr.len, pool->addrstr.data, strerror(errno));
        return NC_ERROR;
    }

    if (p->info.family == AF_UNIX && pool->perm) {
        status = chmod(p->info.addr.un.sun_path, pool->perm);
        if (status < 0) {
            log_error("chmod on %s %d on addr '%.*s' failed: %s",
                      CONN_KIND_AS_STRING(p), p->sd,
                      pool->addrstr.len, pool->addrstr.data, strerror(errno));
            return NC_ERROR;
        }
    }

    status = listen(p->sd, pool->backlog);
    if (status < 0) {
        log_error("listen on %s %d on addr '%.*s' failed: %s",
                  CONN_KIND_AS_STRING(p), p->sd,
                  pool->addrstr.len, pool->addrstr.data, strerror(errno));
        return NC_ERROR;
    }

    status = nc_set_nonblocking(p->sd);
    if (status < 0) {
        log_error("set nonblock on %s %d on addr '%.*s' failed: %s",
                  CONN_KIND_AS_STRING(p), p->sd,
                  pool->addrstr.len, pool->addrstr.data, strerror(errno));
        return NC_ERROR;
    }

    status = event_add_conn(ctx->evb, p);
    if (status < 0) {
        log_error("event add conn %s %d on addr '%.*s' failed: %s",
                  CONN_KIND_AS_STRING(p), p->sd,
                  pool->addrstr.len, pool->addrstr.data,
                  strerror(errno));
        return NC_ERROR;
    }

    status = event_del_out(ctx->evb, p);
    if (status < 0) {
        log_error("event del out %s %d on addr '%.*s' failed: %s",
                  CONN_KIND_AS_STRING(p), p->sd,
                  pool->addrstr.len, pool->addrstr.data,
                  strerror(errno));
        return NC_ERROR;
    }

    return NC_OK;
}

rstatus_t
proxy_each_init(struct server_pool *pool, void *data)
{
    rstatus_t status;
    struct conn *p;

    p = conn_get(pool, pool->redis
            ? NC_CONN_PROXY_REDIS : NC_CONN_PROXY_MEMCACHE);
    if (p == NULL) {
        return NC_ENOMEM;
    }

    status = proxy_listen(pool->ctx, p);
    if (status != NC_OK) {
        p->close(pool->ctx, p);
        return status;
    }

    log_debug(LOG_NOTICE, "%s %d listening on '%.*s' in %s pool %"PRIu32" '%.*s'"
              " with %"PRIu32" servers",
              CONN_KIND_AS_STRING(p), p->sd, pool->addrstr.len,
              pool->addrstr.data, pool->redis ? "redis" : "memcache",
              pool->idx, pool->name.len, pool->name.data,
              array_n(&pool->server));

    return NC_OK;
}

rstatus_t
proxy_init(struct context *ctx, struct server_pools *server_pools)
{
    rstatus_t status;

    if(server_pools_n(server_pools) == 0) {
        log_error("no pools configured, can't initialize proxies");
        return NC_ERROR;
    }

    status = server_pool_each(server_pools, proxy_each_init, NULL);
    if (status != NC_OK) {
        proxy_deinit(ctx, server_pools);
        return status;
    }

    log_debug(LOG_VVERB, "init proxy with %"PRIu32" pools",
              server_pools_n(server_pools));

    return NC_OK;
}

rstatus_t
proxy_each_deinit(struct server_pool *pool, void *data)
{
    struct conn *p;

    p = pool->p_conn;
    if (p != NULL) {
        p->close(pool->ctx, p);
    }

    return NC_OK;
}

void
proxy_deinit(struct context *ctx, struct server_pools *server_pools)
{
    rstatus_t status;
    uint32_t npool;

    npool = server_pools_n(server_pools);

    status = server_pool_each(server_pools, proxy_each_deinit, NULL);
    if (status != NC_OK) {
        return;
    }

    log_debug(LOG_VVERB, "deinit proxy with %"PRIu32" pools", npool);
}

static rstatus_t
proxy_accept(struct context *ctx, struct conn *p)
{
    rstatus_t status;
    struct conn *c;
    int sd;
    struct server_pool *pool = p->owner;

    ASSERT(CONN_KIND_IS_PROXY(p));
    ASSERT(p->sd > 0);
    ASSERT(p->recv_active && p->recv_ready);

    for (;;) {
        sd = accept(p->sd, NULL, NULL);
        if (sd < 0) {
            if (errno == EINTR) {
                log_debug(LOG_VERB, "accept on %s %d not ready - eintr",
                                    CONN_KIND_AS_STRING(p), p->sd);
                continue;
            }

            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ECONNABORTED) {
                log_debug(LOG_VERB, "accept on %s %d not ready - eagain",
                                    CONN_KIND_AS_STRING(p), p->sd);
                p->recv_ready = 0;
                return NC_OK;
            }

            /* 
             * Workaround of https://github.com/twitter/twemproxy/issues/97
             *
             * We should never reach here because the check for conn_ncurr_cconn()
             * against ctx->max_ncconn should catch this earlier in the cycle.
             * If we reach here ignore EMFILE/ENFILE, return NC_OK will enable
             * the server continue to run instead of close the server socket
             *
             * The right solution however, is on EMFILE/ENFILE to mask out IN
             * event on the proxy and mask it back in when some existing
             * connections gets closed
             */
            if (errno == EMFILE || errno == ENFILE) {
                log_debug(LOG_CRIT, "accept on %s %d with max fds %"PRIu32" "
                          "used connections %"PRIu32" max client connections %"PRIu32" "
                          "curr client connections %"PRIu32" failed: %s",
                          CONN_KIND_AS_STRING(p), p->sd,
                          ctx->max_nfd, conn_ncurr_conn(),
                          ctx->max_ncconn, conn_ncurr_cconn(), strerror(errno));

                p->recv_ready = 0;

                return NC_OK;
            }

            log_error("accept on %s %d failed: %s",
                      CONN_KIND_AS_STRING(p), p->sd, strerror(errno));

            return NC_ERROR;
        }

        break;
    }

    const char *conn_kind = conn_kind_s[CONN_KIND_IS_REDIS(p)
                                ? NC_CONN_CLIENT_MEMCACHE
                                : NC_CONN_CLIENT_REDIS];

    if (conn_ncurr_cconn() >= ctx->max_ncconn) {
        log_debug(LOG_CRIT, "client connections %"PRIu32" exceed limit %"PRIu32,
                  conn_ncurr_cconn(), ctx->max_ncconn);
        status = close(sd);
        if (status < 0) {
            log_error("close %s %d failed, ignored: %s",
                      conn_kind, sd, strerror(errno));
        }
        return NC_OK;
    }

    c = conn_get(p->owner, CONN_KIND_IS_REDIS(p)
            ? NC_CONN_CLIENT_REDIS : NC_CONN_CLIENT_MEMCACHE);
    if (c == NULL) {
        log_error("get conn for %s %d from %s %d failed: %s",
                  conn_kind, sd, CONN_KIND_AS_STRING(p), p->sd,
                  strerror(errno));
        status = close(sd);
        if (status < 0) {
            log_error("close %s %d failed, ignored: %s",
                conn_kind, sd, strerror(errno));
        }
        return NC_ENOMEM;
    }
    c->sd = sd;

    stats_pool_incr(ctx, c->owner, client_connections);

    status = nc_set_nonblocking(c->sd);
    if (status < 0) {
        log_error("set nonblock on %s %d from %s %d failed: %s",
                  CONN_KIND_AS_STRING(c), c->sd,
                  CONN_KIND_AS_STRING(p), p->sd, strerror(errno));
        c->close(ctx, c);
        return status;
    }

    if (pool->tcpkeepalive) {
        status = nc_set_tcpkeepalive(c->sd);
        if (status < 0) {
            log_warn("set tcpkeepalive on c %d from p %d failed, ignored: %s",
                     c->sd, p->sd, strerror(errno));
        }
    }

    if (p->info.family == AF_INET || p->info.family == AF_INET6) {
        status = nc_set_tcpnodelay(c->sd);
        if (status < 0) {
            log_warn("set tcpnodelay on %s %d from %s %d failed, ignored: %s",
                     CONN_KIND_AS_STRING(c), c->sd,
                     CONN_KIND_AS_STRING(p), p->sd, strerror(errno));
        }
    }

    status = event_add_conn(ctx->evb, c);
    if (status < 0) {
        log_error("event add conn from %s %d failed: %s",
                  CONN_KIND_AS_STRING(p), p->sd, strerror(errno));
        c->close(ctx, c);
        return status;
    }

    log_debug(LOG_NOTICE, "accepted %s %d on %s %d from '%s'",
              CONN_KIND_AS_STRING(c), c->sd,
              CONN_KIND_AS_STRING(p), p->sd,
              nc_unresolve_peer_desc(c->sd));

    return NC_OK;
}

rstatus_t
proxy_recv(struct context *ctx, struct conn *conn)
{
    rstatus_t status;

    ASSERT(CONN_KIND_IS_PROXY(conn));
    ASSERT(conn->recv_active);

    conn->recv_ready = 1;
    do {
        status = proxy_accept(ctx, conn);
        if (status != NC_OK) {
            return status;
        }
    } while (conn->recv_ready);

    return NC_OK;
}
