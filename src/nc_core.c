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
#include <nc_conf.h>
#include <nc_server.h>
#include <nc_proxy.h>
#include <nc_signal_conn.h>

static uint32_t ctx_id; /* context generation */

static rstatus_t
core_calc_connections(struct context *ctx)
{
    int status;
    struct rlimit limit;

    status = getrlimit(RLIMIT_NOFILE, &limit);
    if (status < 0) {
        log_error("getrlimit failed: %s", strerror(errno));
        return NC_ERROR;
    }

    ctx->max_nfd = (uint32_t)limit.rlim_cur;
    ctx->max_ncconn = ctx->max_nfd - ctx->max_nsconn - RESERVED_FDS;
    log_debug(LOG_NOTICE, "max fds %"PRIu32" max client conns %"PRIu32" "
              "max server conns %"PRIu32"", ctx->max_nfd, ctx->max_ncconn,
              ctx->max_nsconn);

    return NC_OK;
}

static struct context *
core_ctx_create(struct instance *nci)
{
    rstatus_t status;
    struct context *ctx;

    ctx = nc_zalloc(sizeof(*ctx));
    if (ctx == NULL) {
        return NULL;
    }
    ctx->nci = nci;
    ctx->id = ++ctx_id;
    ctx->cf = NULL;
    ctx->stats = NULL;
    ctx->evb = NULL;
    TAILQ_INIT(&ctx->pools);
    TAILQ_INIT(&ctx->replacement_pools);
    ctx->max_timeout = nci->stats_interval;
    ctx->timeout = ctx->max_timeout;
    ctx->max_nfd = 0;
    ctx->max_ncconn = 0;
    ctx->max_nsconn = 0;

    /* parse and create configuration */
    ctx->cf = conf_create(nci->conf_filename);
    if (ctx->cf == NULL) {
        nc_free(ctx);
        return NULL;
    }

    /* initialize server pool from configuration */
    status = server_pools_init(&ctx->pools, &ctx->cf->pool, ctx);
    if (status != NC_OK) {
        conf_destroy(ctx->cf);
        nc_free(ctx);
        return NULL;
    }

    /*
     * Get rlimit and calculate max client connections after we have
     * calculated max server connections
     */
    status = core_calc_connections(ctx);
    if (status != NC_OK) {
        server_pools_deinit(&ctx->pools);
        conf_destroy(ctx->cf);
        nc_free(ctx);
        return NULL;
    }

    /* create stats per server pool */
    ctx->stats = stats_create(nci->stats_port, nci->stats_addr, nci->stats_interval,
                              nci->hostname, &ctx->pools);
    if (ctx->stats == NULL) {
        server_pools_deinit(&ctx->pools);
        conf_destroy(ctx->cf);
        nc_free(ctx);
        return NULL;
    }

    /* initialize event handling for client, proxy and server */
    ctx->evb = event_base_create(EVENT_SIZE, &core_core);
    if (ctx->evb == NULL) {
        stats_destroy(ctx->stats);
        server_pools_deinit(&ctx->pools);
        conf_destroy(ctx->cf);
        nc_free(ctx);
        return NULL;
    }

    /* Create a channel to receive async signals synchronously. */
    ctx->sig_conn = create_signal_listener(ctx);
    if(ctx->sig_conn == NULL) {
        event_base_destroy(ctx->evb);
        stats_destroy(ctx->stats);
        server_pools_deinit(&ctx->pools);
        conf_destroy(ctx->cf);
        nc_free(ctx);
        return NULL;
    }

    /* preconnect? servers in server pool */
    status = server_pools_preconnect(ctx);
    if (status != NC_OK) {
        ctx->sig_conn->close(ctx, ctx->sig_conn);
        server_pools_disconnect(&ctx->pools);
        event_base_destroy(ctx->evb);
        stats_destroy(ctx->stats);
        server_pools_deinit(&ctx->pools);
        conf_destroy(ctx->cf);
        nc_free(ctx);
        return NULL;
    }

    /* initialize proxy per server pool */
    status = proxy_init(ctx, &ctx->pools);
    if (status != NC_OK) {
        ctx->sig_conn->close(ctx, ctx->sig_conn);
        server_pools_disconnect(&ctx->pools);
        event_base_destroy(ctx->evb);
        stats_destroy(ctx->stats);
        server_pools_deinit(&ctx->pools);
        conf_destroy(ctx->cf);
        nc_free(ctx);
        return NULL;
    }

    log_debug(LOG_VVERB, "created ctx %p id %"PRIu32"", ctx, ctx->id);

    return ctx;
}

static void
core_ctx_destroy(struct context *ctx)
{
    log_debug(LOG_VVERB, "destroy ctx %p id %"PRIu32"", ctx, ctx->id);
    if(ctx->sig_conn)
        ctx->sig_conn->close(ctx, ctx->sig_conn);
    proxy_deinit(ctx, &ctx->replacement_pools);
    proxy_deinit(ctx, &ctx->pools);
    server_pools_disconnect(&ctx->pools);
    event_base_destroy(ctx->evb);
    stats_destroy(ctx->stats);
    server_pools_deinit(&ctx->pools);
    conf_destroy(ctx->cf);
    nc_free(ctx);
}

struct context *
core_start(struct instance *nci)
{
    struct context *ctx;

    mbuf_init(nci);
    msg_init();
    conn_init();

    ctx = core_ctx_create(nci);
    if (ctx != NULL) {
        nci->ctx = ctx;
        return ctx;
    }

    conn_deinit();
    msg_deinit();
    mbuf_deinit();

    return NULL;
}

void
core_stop(struct context *ctx)
{
    conn_deinit();
    msg_deinit();
    mbuf_deinit();
    core_ctx_destroy(ctx);
}

static rstatus_t
core_recv(struct context *ctx, struct conn *conn)
{
    rstatus_t status;

    status = conn->recv(ctx, conn);
    if (status != NC_OK) {
        log_debug(LOG_INFO, "recv on %s %d failed: %s",
                  CONN_KIND_AS_STRING(conn),
                  conn->sd, strerror(errno));
    }

    return status;
}

static rstatus_t
core_send(struct context *ctx, struct conn *conn)
{
    rstatus_t status;

    status = conn->send(ctx, conn);
    if (status != NC_OK) {
        log_debug(LOG_INFO, "send on %s %d failed: status: %d errno: %d %s",
                  CONN_KIND_AS_STRING(conn),
                  conn->sd, status, errno, strerror(errno));
    }

    return status;
}

static void
core_close(struct context *ctx, struct conn *conn)
{
    rstatus_t status;
    char *addrstr;

    ASSERT(conn->sd > 0);

    if (CONN_KIND_IS_CLIENT(conn)) {
        addrstr = nc_unresolve_peer_desc(conn->sd);
    } else {
        addrstr = nc_unresolve_addr(conn->addr, conn->addrlen);
    }
    log_debug(LOG_NOTICE, "close %s %d '%s' on event %04"PRIX32" eof %d done "
              "%d rb %zu sb %zu%c %s", CONN_KIND_AS_STRING(conn), conn->sd, addrstr, conn->events,
              conn->eof, conn->done, conn->recv_bytes, conn->send_bytes,
              conn->err ? ':' : ' ', conn->err ? strerror(conn->err) : "");

    status = event_del_conn(ctx->evb, conn);
    if (status < 0) {
        log_warn("event del conn %s %d failed, ignored: %s",
                 CONN_KIND_AS_STRING(conn), conn->sd, strerror(errno));
    }

    conn->close(ctx, conn);
}

static void
core_error(struct context *ctx, struct conn *conn)
{
    rstatus_t status;

    status = nc_get_soerror(conn->sd);
    if (status < 0) {
        log_warn("get soerr on %s %d failed, ignored: %s",
                 CONN_KIND_AS_STRING(conn), conn->sd, strerror(errno));
    }
    conn->err = errno;

    core_close(ctx, conn);
}

static void
core_timeout(struct context *ctx)
{
    for (;;) {
        struct msg *msg;
        struct conn *conn;
        int64_t now, then;

        msg = msg_tmo_min();    /* O(logN) */
        if (msg == NULL) {
            ctx->timeout = ctx->max_timeout;
            return;
        }

        /* skip over req that are in-error or done */

        if (msg->error || msg->done) {
            msg_tmo_delete(msg);
            continue;
        }

        /*
         * timeout expired req and all the outstanding req on the timing
         * out server
         */

        conn = msg->tmo_rbe.data;
        then = msg->tmo_rbe.key;

        now = nc_msec_now();
        if (now < then) {
            int delta = (int)(then - now);
            ctx->timeout = MIN(delta, ctx->max_timeout);
            return;
        }

        log_debug(LOG_INFO, "req %"PRIu64" on s %d timedout", msg->id, conn->sd);

        msg_tmo_delete(msg);
        conn->err = ETIMEDOUT;

        core_close(ctx, conn);
    }
}

rstatus_t
core_core(void *arg, uint32_t events)
{
    rstatus_t status;
    struct conn *conn = arg;
    struct context *ctx;

    if (conn->owner == NULL) {
        log_warn("conn is already unrefed!");
        return NC_OK;
    }

    ctx = conn_to_ctx(conn);

    log_debug(LOG_VVERB, "event %04"PRIX32" on %s %d", events,
              CONN_KIND_AS_STRING(conn), conn->sd);

    conn->events = events;

    /* error takes precedence over read | write */
    if (events & EVENT_ERR) {
        core_error(ctx, conn);
        return NC_ERROR;
    }

    /* read takes precedence over write */
    if (events & EVENT_READ) {
        status = core_recv(ctx, conn);
        if (status != NC_OK || conn->done || conn->err) {
            core_close(ctx, conn);
            return NC_ERROR;
        }
    }

    if (events & EVENT_WRITE) {
        status = core_send(ctx, conn);
        if (status != NC_OK || conn->done || conn->err) {
            core_close(ctx, conn);
            return NC_ERROR;
        }
    }

    return NC_OK;
}

static void config_reload(struct context *ctx) {
    rstatus_t status;

    if(ctx->state == CTX_STATE_STEADY) {
        log_debug(LOG_NOTICE, "reconfiguration requested, proceeding");
        ctx->state = CTX_STATE_RELOADING;
    } else {
        log_debug(LOG_NOTICE, "reconfiguration is already in progress");
        server_pools_log(ctx->nci->log_level, "Current runtime:", &ctx->pools);
        return;
    }

    struct conf *cf = conf_create(ctx->nci->conf_filename);
    if (cf == NULL) {
        log_error("configuration file \"%s\" reload failed: %s",
            ctx->nci->conf_filename, strerror(errno));
        ctx->state = CTX_STATE_STEADY;
        return;
    }

    /* initialize server pool from configuration */
    TAILQ_INIT(&ctx->replacement_pools);
    status = server_pools_init(&ctx->replacement_pools, &cf->pool, ctx);
    if (status != NC_OK) {
        server_pools_deinit(&ctx->replacement_pools);
        conf_destroy(cf);
        ctx->state = CTX_STATE_STEADY;
        return;
    }

    conf_destroy(cf);

    server_pools_log(ctx->nci->log_level, "Effective runtime:", &ctx->pools);
    server_pools_log(ctx->nci->log_level, "Replacement runtime:", &ctx->replacement_pools);

    stats_pause(ctx->stats);

    /*
     * Replacement is a complex project, done in stages. We initiate the
     * replacement process here and mark context as pending until the
     * pool replacement process finishs.
     */
    if(server_pools_kick_replacement(&ctx->pools, &ctx->replacement_pools)) {
        server_pools_deinit(&ctx->replacement_pools);
        ctx->state = CTX_STATE_STEADY;
        stats_resume(ctx->stats);
        log_debug(LOG_NOTICE, "Reload failed, current config:");
        server_pools_log(ctx->nci->log_level,
            "Reload failed, current config:", &ctx->pools);
    } else {
        ctx->state = CTX_STATE_RELOADING;
        server_pools_log(ctx->nci->log_level,
            "Reload in progress; config:", &ctx->pools);
    }
}

static void
handle_accumulated_signals(struct context *ctx) {
    int sig;

    while((sig = conn_next_signal()) > 0) {
        switch(sig) {
        default:
            continue;
        case SIGUSR1:
            /*
             * Reload configuration.
             */
            config_reload(ctx);
        }
    }
}

rstatus_t
core_loop(struct context *ctx)
{
    int nsd;
    int effective_timeout = ctx->timeout;

    /*
     * A logic concerning reload: try to finish the reload
     * (pool replacement) every few dozen milliseconds, and switch
     * back to the normal mode when it finally succeeds.
     */
    switch(ctx->state) {
    case CTX_STATE_STEADY:
        stats_swap(ctx->stats);
        break;
    case CTX_STATE_RELOADING:
        if(server_pools_finish_replacement(&ctx->pools)) {
            int64_t original_start_ts = ctx->stats->start_ts;
            stats_destroy(ctx->stats);
            ctx->stats = stats_create(ctx->nci->stats_port,
                                      ctx->nci->stats_addr,
                                      ctx->nci->stats_interval,
                                      ctx->nci->hostname, &ctx->pools);
            ctx->stats->start_ts = original_start_ts;
            ASSERT(ctx->stats);
            ctx->state = CTX_STATE_STEADY;
            server_pools_log(ctx->nci->log_level,
                "Config reloaded, current runtime:", &ctx->pools);
        } else {
            /* Check whether we finished reloading every 10 milliseconds. */
            effective_timeout = 10;
        }
        break;
    }

    nsd = event_wait(ctx->evb, effective_timeout);
    if (nsd < 0) {
        return nsd;
    }

    handle_accumulated_signals(ctx);

    core_timeout(ctx);

    return NC_OK;
}
