/*
 * twemproxy - A fast and lightweight proxy for memcached protocol.
 * 
 * Copyright (C) 2021, wei huang <wei.kukey@gmail.com>
 * All rights reserved.
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

#include <nc_monitor.h>
#include <nc_array.h>

void
monitor_init(struct server_pool *sp)
{
    if (sp->enable_monitor) {
        array_init(&sp->monitor_conns, CONF_DEFAULT_ARRAY_MONITOR_NUM, sizeof(struct conn *));
    }
}

void
monitor_deinit(struct server_pool *sp)
{
    struct array *monitor_conns = &sp->monitor_conns;

    ASSERT(monitor_conns != NULL);
    
    if (sp->enable_monitor) {
        while (array_n(monitor_conns) > 0) {
            array_pop(monitor_conns);
        }
        array_deinit(monitor_conns);
    }
}

rstatus_t
add_to_monitor(struct conn *c)
{
    struct server_pool *sp = c->owner;

    ASSERT(c->client && sp != NULL);

    struct conn **monitor = array_push(&sp->monitor_conns);
    if (monitor == NULL) {
        return NC_ENOMEM;
    }
    c->monitor_client = 1;
    *monitor = c;

    return NC_OK;
}

void
del_from_monitor(struct conn *c)
{
    uint32_t i;
    struct conn **tmp_conn = NULL;
    struct server_pool *sp = c->owner;
    struct array *a = NULL;

    ASSERT(c->client && c->monitor_client);
    ASSERT(sp != NULL);

    a = &sp->monitor_conns;
    for (i = 0; i < array_n(a); i++) {
        tmp_conn = array_get(a, i);
        if (*tmp_conn == c) {
            array_del(a, i);
            break;
        }
    }
}

struct monitor_data
{
    struct msg *m;
    struct conn *c;
    struct context *ctx;
    struct string *d;
};

static int
monitor_callback(void *conn, void *data)
{
    struct monitor_data *mdata = data;
    struct conn **monitor_conn = conn;
    struct conn *req_c = *monitor_conn;

    struct msg *req = req_get(req_c);
    if (req == NULL) {
        return NC_ENOMEM;
    }
    struct msg *rsp = msg_get(req_c, 0, mdata->c->redis);
    if (rsp == NULL) {
        msg_put(req);
        return NC_ENOMEM;
    }

    req->peer = rsp;
    rsp->peer = req;

    req->done = 1;
    rsp->done = 1;

    if (msg_append_full(rsp, mdata->d->data, mdata->d->len) != NC_OK) {
        msg_put(req);
        msg_put(rsp);
        return NC_ENOMEM;
    }
    req_c->enqueue_outq(mdata->ctx, req_c, req);
    if (event_add_out(mdata->ctx->evb, req_c) != NC_OK) {
        req_c->err = errno;
        req_c->dequeue_outq(mdata->ctx, req_c, req);
        msg_put(req);
        msg_put(rsp);
        return NC_ERROR;
    }
    
    return NC_OK;
}

rstatus_t rsp_send_monitor_msg(struct context *ctx, struct conn *c, struct msg *m)
{
    ASSERT(c->client);

    struct server_pool *sp = c->owner;
    struct string monitor_message = null_string;
    struct monitor_data mdata = {0};
    mdata.m = m;
    mdata.c = c;
    mdata.d = &monitor_message;
    mdata.ctx = ctx;
    struct keypos kpos = {0};
    struct keypos *tmp_kpos = NULL;

    if (c->redis) {
        /* Only Command command has a fake key. */
        if (m->type != MSG_REQ_REDIS_COMMAND) {
            tmp_kpos = array_get(m->keys, 0);

            kpos.start = tmp_kpos->start;
            kpos.end = tmp_kpos->end;
        }
        string_printf(&monitor_message, "+%ld.%06ld [%s] command=%s key0=%.*s\r\n",
                      m->start_ts/1000000, m->start_ts%1000000, 
                      nc_unresolve_peer_desc(c->sd), 
                      (msg_type_string(m->type))->data,
                      kpos.end - kpos.start, kpos.start);

    } else {
        /* FIX ME: add memcached protocol monitor msg */
    }


    array_each(&sp->monitor_conns, monitor_callback, &mdata);

    string_deinit(&monitor_message);
    return NC_OK;
}
