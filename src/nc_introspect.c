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

#include <stdarg.h>

#include <nc_core.h>
#include <nc_conf.h>
#include <nc_server.h>
#include <nc_client.h>
#include <nc_proxy.h>
#include <nc_introspect.h>

struct buffer {
    char *buffer;
    size_t offset;
    size_t size;
};

static void buffer_realloc_upto(struct buffer *b, size_t size) {
    void *p;
    size_t new_size = b->size;
    while((new_size - b->offset) <= size) {
        new_size = new_size ? 2 * new_size : 256;
    }
    p = nc_realloc(b->buffer, new_size);
    ASSERT(p);
    b->buffer = p;
    b->size = new_size;
    b->buffer[b->offset] = '\0';
}

static void buffer_add_bytes(struct buffer *b, char *buf, size_t buf_size) {
    if(b->size - b->offset <= buf_size) {
        buffer_realloc_upto(b, buf_size);
    }
    memcpy(b->buffer + b->offset, buf, buf_size);
    b->offset += buf_size;
    b->buffer[b->offset] = '\0';
}

static void buffer_sprintf(struct buffer *b, const char *fmt, ...) {
    int total;

    va_list ap;

    va_start(ap, fmt);
    total = vsnprintf(b->buffer + b->offset, b->size - b->offset, fmt, ap);
    va_end(ap);

    ASSERT(total >= 0);
    if(b->size - b->offset <= (size_t)total) {
        buffer_realloc_upto(b, (size_t)total);
        va_start(ap, fmt);
        total = vsnprintf(b->buffer + b->offset, b->size - b->offset, fmt, ap);
        va_end(ap);
        ASSERT(b->size - b->offset > (size_t)total);
    }
    b->offset += (size_t)total;
}

struct accumulator {
    struct buffer buf;
    enum fro_format_details detail;
    size_t current_indent_level;
    int seen_server;
};

static void indent(struct accumulator *acc) {
    char buf[16];
    if(acc->current_indent_level < sizeof(buf)) {
        size_t i;
        for(i = 0; i < acc->current_indent_level; i++) {
            buf[i] = ' ';
        }
        buf[i] = '\0';
        buffer_add_bytes(&acc->buf, buf, acc->current_indent_level);
    }
}

static void *
format_morphism(enum nc_morph_elem_type etype, void *elem, void *acc0);

static void format_pool(struct server_pool *pool, struct accumulator *acc) {
#define TO_STRING(_hash, _name)    #_name,
    static const char * const hash_string[] = { HASH_CODEC(TO_STRING) };
    static const char * const dist_string[] = { DIST_CODEC(TO_STRING) };
    static const char * const reload_state_to_string[] = {
        "active",
        "to-shutdown",
        "draining",
        "new-wait4old",
        "new"
    };

    buffer_sprintf(&acc->buf, "%s%s:\n",
        acc->buf.offset ? "\n" : "",
        pool->name.data);
    if(pool->p_conn) {
        buffer_sprintf(&acc->buf, "  listen: %s\n",
            conn_unresolve_descriptive(pool->p_conn));
    }
    buffer_sprintf(&acc->buf, "  redis: %s\n",
                   pool->redis ? "true" : "false");
    buffer_sprintf(&acc->buf, "  distribution: %s\n",
        dist_string[pool->dist_type]);
    buffer_sprintf(&acc->buf, "  hash: %s\n",
        hash_string[pool->key_hash_type]);
    if(pool->hash_tag.len)
        buffer_sprintf(&acc->buf, "  hash_tag: %s\n", pool->hash_tag.data);
    buffer_sprintf(&acc->buf, "  preconnect: %s\n",
                   pool->preconnect ? "true" : "false");
    ASSERT(pool->reload_state <= sizeof(reload_state_to_string)/sizeof(reload_state_to_string[0]));
    buffer_sprintf(&acc->buf, "  reload_state: %s\n",
                    reload_state_to_string[pool->reload_state]);
    if(acc->detail & FRO_SERVERS)
        buffer_sprintf(&acc->buf, "  servers:\n");
}

static void format_server(struct server *server, struct accumulator *acc) {
    buffer_sprintf(&acc->buf, "- %s%s%s\n",
        server->pname.data,
        server->name.len ? "   " : "",
        server->name.data);
}

static void format_connection(struct conn *conn, struct accumulator *acc) {
    if(CONN_KIND_IS_CLIENT(conn)) {   
        buffer_sprintf(&acc->buf, "- %s\n",
            nc_unresolve_peer_desc(conn->sd));
    } else if(CONN_KIND_IS_SERVER(conn)) {
        if(!(acc->detail & FRO_SERVERS))
                return;
        if(conn->family == AF_UNIX) {
            buffer_sprintf(&acc->buf, "- %s\n",
                ((struct sockaddr_un *)conn->addr)->sun_path);
        } else {
            buffer_sprintf(&acc->buf, "- %s\n",
                nc_unresolve_addr((struct sockaddr *)&conn->addr,
                                  conn->addrlen));
        }
    } else if(CONN_KIND_IS_PROXY(conn)) {
        /* Do not print anything, printed separately. */
        return;
    }
}

/*
 * Display:
 *   pool "a"
 *     server1
 *     server2
 *     client1
 *     client2
 */
static void *
format_morphism(enum nc_morph_elem_type etype, void *elem, void *acc0) {
    struct accumulator *acc = acc0;
    struct conn *conn;

    switch(etype) {
    case NC_ELEMENT_IS_POOL:
        acc->current_indent_level = 0;
        acc->seen_server = 0;
        format_pool(elem, acc);
        break;
    case NC_ELEMENT_IS_SERVER:
        acc->current_indent_level = 4;
        acc->seen_server = 1;
        if(!(acc->detail & FRO_SERVERS))
            break;
        indent(acc);
        format_server(elem, acc);
        break;
    case NC_ELEMENT_IS_CONNECTION:
        conn = elem;
        if(!(acc->detail & FRO_SERVERS))
            break;
        if(acc->seen_server) {
            acc->current_indent_level = 4;
        } else {
            acc->current_indent_level = 0;
        }
        format_connection(elem, acc);
        break;
    }

    return acc;
}

/*
 * Format the pools/servers/connections as a human-readable string.
 * WARNING: might not turn out a properly formatted YAML; use for debug only.
 */
char *
format_runtime_objects(struct context *ctx, struct server_pools *server_pools, enum fro_format_details detail) {
    struct accumulator acc;

    memset(&acc, 0, sizeof(acc));
    acc.detail = detail;

    server_pools_fold(server_pools, format_morphism, &acc);

    return acc.buf.buffer;
}

/*
 * Send the formatted layout straight to the log.
 */
void
log_runtime_objects(int level, struct context *ctx, struct server_pools *server_pools, enum fro_format_details detail) {
    char *formatted_objects, *ptr, *eol;

    if(ctx->nci->log_level < level)
        return;

    formatted_objects = format_runtime_objects(ctx, server_pools, detail);

    ptr = formatted_objects;
    while((eol = strchr(ptr, '\n')) != NULL) {
        *eol = '\0';
        log_debug(level, "%s", ptr);
        ptr = eol + 1;
    }

    nc_free(formatted_objects);
}

