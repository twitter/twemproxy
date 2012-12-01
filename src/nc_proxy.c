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

#include <sys/un.h>

#include <nc_core.h>
#include <nc_server.h>
#include <nc_event.h>
#include <nc_proxy.h>

void
proxy_ref(struct conn *conn, void *owner)
{
    struct server_pool *pool = owner;

    ASSERT(!conn->client && conn->proxy);
    ASSERT(conn->owner == NULL);

    conn->family = pool->family;
    conn->addrlen = pool->addrlen;
    conn->addr = pool->addr;

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

    ASSERT(!conn->client && conn->proxy);
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

    ASSERT(!conn->client && conn->proxy);

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
        log_error("close p %d failed, ignored: %s", conn->sd, strerror(errno));
    }
    conn->sd = -1;

    conn_put(conn);
}

static rstatus_t
proxy_reuse(struct conn *p)
{
    rstatus_t status;
    struct sockaddr_un *un;

    switch (p->family) {
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
        un = (struct sockaddr_un *) p->addr;
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

    ASSERT(p->proxy);

    p->sd = socket(p->family, SOCK_STREAM, 0);
    if (p->sd < 0) {
        log_error("socket failed: %s", strerror(errno));
        return NC_ERROR;
    }

    status = proxy_reuse(p);
    if (status < 0) {
        log_error("reuse of addr '%.*s' for listening on p %d failed: %s",
                  pool->addrstr.len, pool->addrstr.data, p->sd,
                  strerror(errno));
        return NC_ERROR;
    }

    status = bind(p->sd, p->addr, p->addrlen);
    if (status < 0) {
        log_error("bind on p %d to addr '%.*s' failed: %s", p->sd,
                  pool->addrstr.len, pool->addrstr.data, strerror(errno));
        return NC_ERROR;
    }

    status = listen(p->sd, pool->backlog);
    if (status < 0) {
        log_error("listen on p %d on addr '%.*s' failed: %s", p->sd,
                  pool->addrstr.len, pool->addrstr.data, strerror(errno));
        return NC_ERROR;
    }

    status = nc_set_nonblocking(p->sd);
    if (status < 0) {
        log_error("set nonblock on p %d on addr '%.*s' failed: %s", p->sd,
                  pool->addrstr.len, pool->addrstr.data, strerror(errno));
        return NC_ERROR;
    }

    status = event_add_conn(ctx->ep, p);
    if (status < 0) {
        log_error("event add conn e %d p %d on addr '%.*s' failed: %s",
                  ctx->ep, p->sd, pool->addrstr.len, pool->addrstr.data,
                  strerror(errno));
        return NC_ERROR;
    }

    status = event_del_out(ctx->ep, p);
    if (status < 0) {
        log_error("event del out e %d p %d on addr '%.*s' failed: %s",
                  ctx->ep, p->sd, pool->addrstr.len, pool->addrstr.data,
                  strerror(errno));
        return NC_ERROR;
    }

    return NC_OK;
}

rstatus_t
proxy_each_init(void *elem, void *data)
{
    rstatus_t status;
    struct server_pool *pool = elem;
    struct conn *p;

    p = conn_get_proxy(pool);
    if (p == NULL) {
        return NC_ENOMEM;
    }

    status = proxy_listen(pool->ctx, p);
    if (status != NC_OK) {
        p->close(pool->ctx, p);
        return status;
    }

    log_debug(LOG_NOTICE, "p %d listening on '%.*s' in %s pool %"PRIu32" '%.*s'"
              " with %"PRIu32" servers", p->sd, pool->addrstr.len,
              pool->addrstr.data, pool->redis ? "redis" : "memcache",
              pool->idx, pool->name.len, pool->name.data,
              array_n(&pool->server));

    return NC_OK;
}

rstatus_t
proxy_init(struct context *ctx)
{
    rstatus_t status;

    ASSERT(array_n(&ctx->pool) != 0);

    status = array_each(&ctx->pool, proxy_each_init, NULL);
    if (status != NC_OK) {
        proxy_deinit(ctx);
        return status;
    }

    log_debug(LOG_VVERB, "init proxy with %"PRIu32" pools",
              array_n(&ctx->pool));

    return NC_OK;
}

rstatus_t
proxy_each_deinit(void *elem, void *data)
{
    struct server_pool *pool = elem;
    struct conn *p;

    p = pool->p_conn;
    if (p != NULL) {
        p->close(pool->ctx, p);
    }

    return NC_OK;
}

void
proxy_deinit(struct context *ctx)
{
    rstatus_t status;

    ASSERT(array_n(&ctx->pool) != 0);

    status = array_each(&ctx->pool, proxy_each_deinit, NULL);
    if (status != NC_OK) {
        return;
    }

    log_debug(LOG_VVERB, "deinit proxy with %"PRIu32" pools",
              array_n(&ctx->pool));
}

static rstatus_t
proxy_accept(struct context *ctx, struct conn *p)
{
    rstatus_t status;
    struct conn *c;
    int sd;

    ASSERT(p->proxy && !p->client);
    ASSERT(p->sd > 0);
    ASSERT(p->recv_active && p->recv_ready);

    for (;;) {
        sd = accept(p->sd, NULL, NULL);
        if (sd < 0) {
            if (errno == EINTR) {
                log_debug(LOG_VERB, "accept on p %d not ready - eintr", p->sd);
                continue;
            }

            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                log_debug(LOG_VERB, "accept on p %d not ready - eagain", p->sd);
                p->recv_ready = 0;
                return NC_OK;
            }

            /*
             * FIXME: On EMFILE or ENFILE mask out IN event on the proxy; mask
             * it back in when some existing connection gets closed
             */

            log_error("accept on p %d failed: %s", p->sd, strerror(errno));
            return NC_ERROR;
        }

        break;
    }

    c = conn_get(p->owner, true, p->redis);
    if (c == NULL) {
        log_error("get conn for c %d from p %d failed: %s", sd, p->sd,
                  strerror(errno));
        status = close(sd);
        if (status < 0) {
            log_error("close c %d failed, ignored: %s", sd, strerror(errno));
        }
        return NC_ENOMEM;
    }
    c->sd = sd;

    stats_pool_incr(ctx, c->owner, client_connections);

    status = nc_set_nonblocking(c->sd);
    if (status < 0) {
        log_error("set nonblock on c %d from p %d failed: %s", c->sd, p->sd,
                  strerror(errno));
        c->close(ctx, c);
        return status;
    }

    if (p->family == AF_INET || p->family == AF_INET6) {
        status = nc_set_tcpnodelay(c->sd);
        if (status < 0) {
            log_warn("set tcpnodelay on c %d from p %d failed, ignored: %s",
                     c->sd, p->sd, strerror(errno));
        }
    }

    status = event_add_conn(ctx->ep, c);
    if (status < 0) {
        log_error("event add conn of c %d from p %d failed: %s", c->sd, p->sd,
                  strerror(errno));
        c->close(ctx, c);
        return status;
    }

    log_debug(LOG_NOTICE, "accepted c %d on p %d from '%s'", c->sd, p->sd,
              nc_unresolve_peer_desc(c->sd));

    return NC_OK;
}

rstatus_t
proxy_recv(struct context *ctx, struct conn *conn)
{
    rstatus_t status;

    ASSERT(conn->proxy && !conn->client);
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
