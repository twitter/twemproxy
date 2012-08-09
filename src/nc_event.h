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

#define NC_EVENT_SIZE 1024
#define EV_READ 0xff
#define EV_WRITE 0xff00
#define EV_ERR 0xff0000

#ifdef NC_HAVE_KQUEUE
struct evbase {
    int                  kq;
    struct kevent        *changes;
    struct kevent        *kevents;
    int                  n_changes;
    int                  nevent;
    void (*callback_fp)(void *, uint32_t);
};
#endif
#ifdef NC_HAVE_EPOLL
struct evbase {
    int                   ep;
    int                   nevent;
    struct epoll_event    *event;
    void (*callback_fp)(void *, uint32_t);
};
#endif

struct evbase *evbase_create(int size, void (*callback_fp)(void *, uint32_t));
void evbase_destroy(struct evbase *evb);

int event_add_out(struct evbase *evb, struct conn *c);
int event_del_out(struct evbase *evb, struct conn *c);
int event_add_conn(struct evbase *evb, struct conn *c);
int event_del_conn(struct evbase *evb, struct conn *c);
int event_wait(struct evbase *evb, int timeout);
int event_add_st(struct evbase *evb, int fd);

#endif /* _NC_EVENT_H */
