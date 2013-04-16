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

#include <nc_core.h>

#ifdef NC_HAVE_EPOLL

#include <sys/epoll.h>

struct evbase *
evbase_create(int nevent, void (*callback_fp)(void *, uint32_t))
{
    struct evbase *evb;
    int status, ep;
    struct epoll_event *event;

    if (nevent <= 0) {
        log_error("nevent has to be positive %d", nevent);
        return NULL;
    }

    ep = epoll_create(nevent);
    if (ep < 0) {
        log_error("epoll create of size %d failed: %s", nevent, strerror(errno));
        return NULL;
    }

    event = nc_calloc(nevent, sizeof(*event));
    if (event == NULL) {
        status = close(ep);
        if (status < 0) {
            log_error("close e %d failed, ignored: %s", ep, strerror(errno));
        }
        return NULL;
    }

    evb = nc_alloc(sizeof(*evb));
    if (evb == NULL) {
        nc_free(event);
        status = close(ep);
        if (status < 0) {
            log_error("close e %d failed, ignored: %s", ep, strerror(errno));
        }
        return NULL;

    }

    evb->nevent = nevent;
    evb->ep = ep;
    evb->event = event;
    evb->callback_fp = callback_fp;

    log_debug(LOG_INFO, "e %d with nevent %d", evb->ep,
              evb->nevent);

    return evb;
}

void
evbase_destroy(struct evbase *evb)
{
    int status;

    if (evb == NULL) {
        return;
    }

    ASSERT(evb->ep >= 0);

    nc_free(evb->event);

    status = close(evb->ep);
    if (status < 0) {
        log_error("close e %d failed, ignored: %s", evb->ep, strerror(errno));
    }
    nc_free(evb);
}

int
event_add_out(struct evbase *evb, struct conn *c)
{
    int status;
    struct epoll_event event;
    int ep = evb->ep;

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
}

int
event_del_out(struct evbase *evb, struct conn *c)
{
    int status;
    struct epoll_event event;
    int ep = evb->ep;

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
}

int
event_add_conn(struct evbase *evb, struct conn *c)
{
    int status;
    struct epoll_event event;
    int ep = evb->ep;

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
}

int
event_del_conn(struct evbase *evb, struct conn *c)
{
    int status;
    int ep = evb->ep;

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
}

int
event_wait(struct evbase *evb, int timeout)
{
    int nsd, i;
    uint32_t events = 0;
    int ep = evb->ep;
    struct epoll_event *event = evb->event;
    int nevent = evb->nevent;
    void (*callback_fp)(void *, uint32_t) = evb->callback_fp;

    ASSERT(ep > 0);
    ASSERT(event != NULL);
    ASSERT(nevent > 0);

    for (;;) {
        nsd = epoll_wait(ep, event, nevent, timeout);
        if (nsd > 0) {
            for (i = 0; i < nsd; i++) {
                struct epoll_event *ev = &evb->event[i];

                events = 0;
                if (ev->events & EPOLLERR) {
                    events |= EV_ERR;
                }

                if (ev->events & EPOLLIN) {
                    events |= EV_READ;
                }

                if (ev->events & EPOLLOUT) {
                    events |= EV_WRITE;
                }

                if (callback_fp != NULL) {
                    (*callback_fp)((void *) ev->data.ptr, events);
                }
            }
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
    NOT_REACHED();
}

int
event_add_st(struct evbase *evb, int fd)
{
    int status;
    struct epoll_event ev;

    ev.data.fd = fd;
    ev.events = EPOLLIN;

    status = epoll_ctl(evb->ep, EPOLL_CTL_ADD, fd, &ev);
    if (status < 0) {
        log_error("epoll ctl on e %d sd %d failed: %s", evb->ep, fd,
                  strerror(errno));
        return status;
    }

    return status;
}

#endif /* NC_HAVE_EPOLL */
