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
#include <signal.h>

#include <nc_core.h>
#include <nc_connection.h>
#include <nc_signal.h>
#include <nc_signal_conn.h>

struct conn *create_signal_listener(struct context *ctx) {
    rstatus_t status;
    struct conn *conn;

    conn = conn_get(ctx, NC_CONN_SIGNAL);
    if(conn == NULL) {
        return NULL;
    }

    conn->sd = signal_get_fd();
    ASSERT(conn->sd >= 0);

    status = nc_set_nonblocking(conn->sd);
    if(status != NC_OK) {
        log_error("set nonblock on %d failed: %s", conn->sd, strerror(errno));
        conn->close(ctx, conn);
        return NULL;
    }

    status = event_add_conn(ctx->evb, conn);
    if(status != NC_OK) {
        log_error("event_add_conn %d failed: %s", conn->sd, strerror(errno));
        conn->close(ctx, conn);
        return NULL;
    }

    status = event_del_out(ctx->evb, conn);
    if(status != NC_OK) {
        log_error("event_del_in %d failed: %s", conn->sd, strerror(errno));
        conn->close(ctx, conn);
        return NULL;
    }

    log_debug(LOG_VVERB, "created signal listener as %s %d",
        CONN_KIND_AS_STRING(conn), conn->sd);

    return conn;
}

void conn_signal_close(struct context *ctx, struct conn *conn) {
    /*
     * This conn_signal_close() code is never really invoked.
     */
    if(conn->sd >= 0) {
        close(conn->sd);
        conn->sd = -1;
    }

    conn->unref(conn);
    conn_put(conn);
}

void
conn_signal_ref(struct conn *conn, void *owner) {
    conn->owner = owner;
}

void
conn_signal_unref(struct conn *conn) {
    conn->owner = NULL;
}

static uint32_t signals_received;

int
conn_next_signal() {

    if(signals_received) {
        for(int i = SIGUSR1; i <= SIGUSR2; i++) {
            if(signals_received & (1 << (i-1))) {
                signals_received &= ~(1 << (i - 1));
                return i;
            }
        }
        signals_received = 0;
    }

    return 0;
}

rstatus_t
conn_signal_recv(struct context *ctx, struct conn *conn) {
    ASSERT(conn->sd >= 0);
    int c;

    log_debug(LOG_VVVERB, "got fd event after signal on %d", conn->sd);

    ssize_t rd = read(conn->sd, &c, 1);
    if(rd == 1) {
        signals_received |= 1 << (c-1);
    }

    return NC_OK;
}

