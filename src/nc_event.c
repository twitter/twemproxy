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

#include <unistd.h>
#ifdef NC_HAVE_EPOLL
#include <sys/epoll.h>
#endif
#ifdef NC_HAVE_KQUEUE
#include <sys/event.h>
#endif
#include <nc_core.h>
#include <nc_event.h>

int
event_init(struct context *ctx, int size)
{
#ifdef NC_HAVE_EPOLL
    int status, ep;
    struct epoll_event *event;

    ASSERT(ctx->ep < 0);
    ASSERT(ctx->nevent != 0);
    ASSERT(ctx->event == NULL);

    ep = epoll_create(size);
    if (ep < 0) {
        log_error("epoll create of size %d failed: %s", size, strerror(errno));
        return -1;
    }

    event = nc_calloc(ctx->nevent, sizeof(*ctx->event));
    if (event == NULL) {
        status = close(ep);
        if (status < 0) {
            log_error("close e %d failed, ignored: %s", ep, strerror(errno));
        }
        return -1;
    }

    ctx->ep = ep;
    ctx->event = event;

    log_debug(LOG_INFO, "e %d with nevent %d timeout %d", ctx->ep,
              ctx->nevent, ctx->timeout);

    return 0;
#endif
#ifdef NC_HAVE_KQUEUE
    int status, kq;
    struct kevent *changes, *kevents;

    ASSERT(ctx->kq < 0);
    ASSERT(ctx->nevent != 0);
    ASSERT(ctx->changes == NULL);
    ASSERT(ctx->kevents == NULL);

    /* Initialize the kernel queue */
    if ((kq = kqueue()) == -1) {
        log_error("kernel queue create failed: %s", kq, strerror(errno));
        return -1;
    }

    changes = nc_calloc(ctx->nevent, sizeof(struct kevent));
    if (changes == NULL) {
        status = close(kq);
        if (status < 0) {
            log_error("close kq %d failed, ignored: %s", kq, strerror(errno));
        }
        return -1;
    }

    kevents = nc_calloc(ctx->nevent, sizeof(struct kevent));
    if (kevents == NULL) {
        status = close(kq);
        if (status < 0) {
            log_error("close kq %d failed, ignored: %s", kq, strerror(errno));
        }
        return -1;
    }

    ctx->kq = kq;
    ctx->changes = changes;
    ctx->kevents  = kevents;

    log_debug(LOG_INFO, "kq %d with nevent %d timeout %d", ctx->kq,
              ctx->nevent, ctx->timeout);

    return 0;

#endif
}

void
event_deinit(struct context *ctx)
{
#ifdef NC_HAVE_EPOLL
    int status;

    ASSERT(ctx->ep >= 0);

    nc_free(ctx->event);

    status = close(ctx->ep);
    if (status < 0) {
        log_error("close e %d failed, ignored: %s", ctx->ep, strerror(errno));
    }
    ctx->ep = -1;
#endif
#ifdef NC_HAVE_KQUEUE
    int status;

    ASSERT(ctx->kq >= 0);

    nc_free(ctx->changes);
    nc_free(ctx->kevents);

    status = close(ctx->kq);
    if (status < 0) {
        log_error("close kq %d failed, ignored: %s", ctx->kq, strerror(errno));
    }
    ctx->kq = -1;
#endif
}

int
event_add_out(struct context *ctx, struct conn *c)
{
#ifdef NC_HAVE_EPOLL
    int status;
    struct epoll_event event;
    int ep = ctx->ep;

    ASSERT(ep > 0);
    ASSERT(c != NULL);
    ASSERT(c->sd > 0);
    ASSERT(c->recv_active);

    if (c->send_active) {
        return 0;
    }

    event.events = (uint32_t)(EPOLLIN | EPOLLOUT | EPOLLET);
    event.data.ptr = c;

    status = epoll_ctl(ep, EPOLL_CTL_MOD, c->sd, &event);
    if (status < 0) {
        log_error("epoll ctl on e %d sd %d failed: %s", ep, c->sd,
                  strerror(errno));
    } else {
        c->send_active = 1;
    }

    return status;
#endif
#ifdef NC_HAVE_KQUEUE
    struct kevent *event;
    int kq = ctx->kq;

    ASSERT(kq > 0);
    ASSERT(c != NULL);
    ASSERT(c->sd > 0);
    ASSERT(c->recv_active);
    ASSERT(ctx->n_changes < ctx->nevent);

    if (c->send_active) {
        return 0;
    }

    event = &ctx->changes[(ctx->n_changes)++];
    memset(event, 0, sizeof(struct kevent));

    event->ident = c->sd;
    event->filter = EVFILT_WRITE;
    event->flags = EV_ADD | EV_CLEAR;
    event->udata = (void *) c;

    c->send_active = 1;

    return 0;
#endif
}

int
event_del_out(struct context *ctx, struct conn *c)
{
#ifdef NC_HAVE_EPOLL
    int status;
    struct epoll_event event;
    int ep = ctx->ep;

    ASSERT(ep > 0);
    ASSERT(c != NULL);
    ASSERT(c->sd > 0);
    ASSERT(c->recv_active);

    if (!c->send_active) {
        return 0;
    }

    event.events = (uint32_t)(EPOLLIN | EPOLLET);
    event.data.ptr = c;

    status = epoll_ctl(ep, EPOLL_CTL_MOD, c->sd, &event);
    if (status < 0) {
        log_error("epoll ctl on e %d sd %d failed: %s", ep, c->sd,
                  strerror(errno));
    } else {
        c->send_active = 0;
    }

    return status;
#endif
#ifdef NC_HAVE_KQUEUE
    struct kevent *event;
    int kq = ctx->kq;

    ASSERT(kq > 0);
    ASSERT(c != NULL);
    ASSERT(c->sd > 0);
    ASSERT(c->recv_active);
    ASSERT(ctx->n_changes < ctx->nevent);

    if (!c->send_active) {
        return 0;
    }
    
    event = &ctx->changes[(ctx->n_changes)++];
    memset(event, 0, sizeof(struct kevent));

    event->ident = c->sd;
    event->filter = EVFILT_WRITE;
    event->flags = EV_DELETE;
    event->udata = (void *) c;

    c->send_active = 0;

    return 0;
#endif
}

int
event_add_conn(struct context *ctx, struct conn *c)
{
#ifdef NC_HAVE_EPOLL
    int status;
    struct epoll_event event;
    int ep = ctx->ep;

    ASSERT(ep > 0);
    ASSERT(c != NULL);
    ASSERT(c->sd > 0);

    event.events = (uint32_t)(EPOLLIN | EPOLLOUT | EPOLLET);
    event.data.ptr = c;

    status = epoll_ctl(ep, EPOLL_CTL_ADD, c->sd, &event);
    if (status < 0) {
        log_error("epoll ctl on e %d sd %d failed: %s", ep, c->sd,
                  strerror(errno));
    } else {
        c->send_active = 1;
        c->recv_active = 1;
    }

    return status;
#endif
#ifdef NC_HAVE_KQUEUE
    struct kevent *event;
    int kq = ctx->kq;

    ASSERT(kq > 0);
    ASSERT(c != NULL);
    ASSERT(c->sd > 0);
    ASSERT(ctx->n_changes < ctx->nevent);

    event = &ctx->changes[(ctx->n_changes)++];
    memset(event, 0, sizeof(struct kevent));

    event->ident = c->sd;
    event->filter = EVFILT_READ;
    event->flags = EV_ADD | EV_CLEAR;
    event->udata = (void *) c;

    c->recv_active = 1;

    event_add_out(ctx, c);
    c->send_active = 1;

    return 0;
#endif
}

int
event_del_conn(struct context *ctx, struct conn *c)
{
#ifdef NC_HAVE_EPOLL
    int status;
    int ep = ctx->ep;

    ASSERT(ep > 0);
    ASSERT(c != NULL);
    ASSERT(c->sd > 0);

    status = epoll_ctl(ep, EPOLL_CTL_DEL, c->sd, NULL);
    if (status < 0) {
        log_error("epoll ctl on e %d sd %d failed: %s", ep, c->sd,
                  strerror(errno));
    } else {
        c->recv_active = 0;
        c->send_active = 0;
    }

    return status;
#endif
#ifdef NC_HAVE_KQUEUE
    struct kevent *event;
    int kq = ctx->kq;

    ASSERT(kq > 0);
    ASSERT(c != NULL);
    ASSERT(c->sd > 0);
    ASSERT(ctx->n_changes < ctx->nevent);

    event = &ctx->changes[(ctx->n_changes)++];
    memset(event, 0, sizeof(struct kevent));

    event->ident = c->sd;
    event->filter = EVFILT_READ | EVFILT_WRITE;
    event->flags = EV_DELETE;
    event->udata = (void *) c;

    c->send_active = 0;
    c->recv_active = 0;

    return 0;
#endif
}

int
event_wait(struct context *ctx)
{
#ifdef NC_HAVE_EPOLL
    int nsd;
    int ep = ctx->ep;
    int event = ctx->event;
    int nevent = ctx->nevent;
    int timeout = ctx->timeout;

    ASSERT(ep > 0);
    ASSERT(event != NULL);
    ASSERT(nevent > 0);

    for (;;) {
        nsd = epoll_wait(ep, event, nevent, timeout);
        if (nsd > 0) {
            return nsd;
        }

        if (nsd == 0) {
            if (timeout == -1) {
               log_error("epoll wait on e %d with %d events and %d timeout "
                         "returned no events", ep, nevent, timeout);
                return -1;
            }

            return 0;
        }

        if (errno == EINTR) {
            continue;
        }

        log_error("epoll wait on e %d with %d events failed: %s", ep, nevent,
                  strerror(errno));

        return -1;
    }
#endif
#ifdef NC_HAVE_KQUEUE
    int nsd;
    int kq = ctx->kq;
    int nevent = ctx->nevent;
    int timeout = ctx->timeout;
    int n_changes = ctx->n_changes;
    struct timespec ts = nc_millisec_to_timespec(timeout);

    ASSERT(kq > 0);
    ASSERT(nevent > 0);

    ctx->n_changes = 0;

    for (;;) {
        nsd = kevent(kq, ctx->changes, n_changes, ctx->kevents, nevent, &ts);
        if (nsd > 0) {
            return nsd;
        }

        if (nsd == 0) {
            if (timeout == -1) {
               log_error("kqueue on kq %d with %d events and %d timeout "
                         "returned no events", kq, nevent, timeout);
                return -1;
            }

            return 0;
        }

        if (errno == EINTR) {
            continue;
        }

        log_error("kevent on kq %d with %d events failed: %s", kq, nevent,
                  strerror(errno));

        return -1;
    }
#endif
    NOT_REACHED();
}
