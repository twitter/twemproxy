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

#ifndef _NC_EVENT_H_
#define _NC_EVENT_H_

#include <nc_core.h>

#define EVENT_SIZE  1024

#define EVENT_READ  0x0000ff
#define EVENT_WRITE 0x00ff00
#define EVENT_ERR   0xff0000

typedef int (*event_cb_t)(void *, uint32_t);
typedef void (*event_stats_cb_t)(void *, void *);

#ifdef NC_HAVE_KQUEUE

struct event_base {
    int           kq;          /* kernel event queue descriptor */

    struct kevent *change;     /* change[] - events we want to monitor */
    int           nchange;     /* # change */

    struct kevent *event;      /* event[] - events that were triggered */
    int           nevent;      /* # event */
    int           nreturned;   /* # event placed in event[] */
    int           nprocessed;  /* # event processed from event[] */

    event_cb_t    cb;          /* event callback */
};

#elif NC_HAVE_EPOLL

struct event_base {
    int                ep;      /* epoll descriptor */

    struct epoll_event *event;  /* event[] - events that were triggered */
    int                nevent;  /* # event */

    event_cb_t         cb;      /* event callback */
};

#elif NC_HAVE_EVENT_PORTS

#include <port.h>

struct event_base {
    int          evp;     /* event port descriptor */

    port_event_t *event;  /* event[] - events that were triggered */
    int          nevent;  /* # event */

    event_cb_t   cb;      /* event callback */
};

#else
# error missing scalable I/O event notification mechanism
#endif

struct event_base *event_base_create(int size, event_cb_t cb);
void event_base_destroy(struct event_base *evb);

int event_add_in(struct event_base *evb, struct conn *c);
int event_del_in(struct event_base *evb, struct conn *c);
int event_add_out(struct event_base *evb, struct conn *c);
int event_del_out(struct event_base *evb, struct conn *c);
int event_add_conn(struct event_base *evb, struct conn *c);
int event_del_conn(struct event_base *evb, struct conn *c);
int event_wait(struct event_base *evb, int timeout);
void event_loop_stats(event_stats_cb_t cb, void *arg);

#endif /* _NC_EVENT_H */
