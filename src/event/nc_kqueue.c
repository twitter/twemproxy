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

#ifdef NC_HAVE_KQUEUE

#include <sys/event.h>

struct event_base *
event_base_create(int nevent, event_cb_t cb)
{
    struct event_base *evb;
    int status, kq;
    struct kevent *change, *event;

    ASSERT(nevent > 0);

    kq = kqueue();
    if (kq < 0) {
        log_error("kqueue failed: %s", strerror(errno));
        return NULL;
    }

    change = nc_calloc(nevent, sizeof(*change));
    if (change == NULL) {
        status = close(kq);
        if (status < 0) {
            log_error("close kq %d failed, ignored: %s", kq, strerror(errno));
        }
        return NULL;
    }

    event = nc_calloc(nevent, sizeof(*event));
    if (event == NULL) {
        nc_free(change);
        status = close(kq);
        if (status < 0) {
            log_error("close kq %d failed, ignored: %s", kq, strerror(errno));
        }
        return NULL;
    }

    evb = nc_alloc(sizeof(*evb));
    if (evb == NULL) {
        nc_free(change);
        nc_free(event);
        status = close(kq);
        if (status < 0) {
            log_error("close kq %d failed, ignored: %s", kq, strerror(errno));
        }
        return NULL;
    }

    evb->kq = kq;
    evb->change = change;
    evb->nchange = 0;
    evb->event = event;
    evb->nevent = nevent;
    evb->nreturned = 0;
    evb->nprocessed = 0;
    evb->cb = cb;

    log_debug(LOG_INFO, "kq %d with nevent %d", evb->kq, evb->nevent);

    return evb;
}

void
event_base_destroy(struct event_base *evb)
{
    int status;

    if (evb == NULL) {
        return;
    }

    ASSERT(evb->kq > 0);

    nc_free(evb->change);
    nc_free(evb->event);

    status = close(evb->kq);
    if (status < 0) {
        log_error("close kq %d failed, ignored: %s", evb->kq, strerror(errno));
    }
    evb->kq = -1;

    nc_free(evb);
}

static void
eliminate_pending_events(struct event_base *evb, struct conn *c, int16_t filter) {
    for (int i = evb->nprocessed + 1; i < evb->nreturned; i++) {
        struct kevent *ev = &evb->event[i];
        if (ev->udata == c && (!filter || filter == ev->filter)) {
            ev->flags = 0;
            ev->filter = 0;
            break;
        }
    }
}

int
event_add_in(struct event_base *evb, struct conn *c)
{
    struct kevent *event;

    ASSERT(evb->kq > 0);
    ASSERT(c != NULL);
    ASSERT(c->sd > 0);
    ASSERT(evb->nchange < evb->nevent);

    if (c->recv_active) {
        return 0;
    }

    event = &evb->change[evb->nchange++];
    EV_SET(event, c->sd, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, c);

    c->recv_active = 1;

    return 0;
}

int
event_del_in(struct event_base *evb, struct conn *c)
{
    struct kevent *event;

    ASSERT(evb->kq > 0);
    ASSERT(c != NULL);
    ASSERT(c->sd > 0);
    ASSERT(evb->nchange < evb->nevent);

    if (!c->recv_active) {
        return 0;
    }

    event = &evb->change[evb->nchange++];
    EV_SET(event, c->sd, EVFILT_READ, EV_DELETE, 0, 0, c);

    c->recv_active = 0;

    /*
     * Eliminate subsequent READ events, if we have disabled READ during
     * processing one of the previous ones.
     */
    eliminate_pending_events(evb, c, EVFILT_READ);

    return 0;
}

int
event_add_out(struct event_base *evb, struct conn *c)
{
    struct kevent *event;

    ASSERT(evb->kq > 0);
    ASSERT(c != NULL);
    ASSERT(c->sd > 0);
    ASSERT(evb->nchange < evb->nevent);

    if (c->send_active) {
        return 0;
    }

    event = &evb->change[evb->nchange++];
    EV_SET(event, c->sd, EVFILT_WRITE, EV_ADD | EV_CLEAR, 0, 0, c);

    c->send_active = 1;

    return 0;
}

int
event_del_out(struct event_base *evb, struct conn *c)
{
    struct kevent *event;

    ASSERT(evb->kq > 0);
    ASSERT(c != NULL);
    ASSERT(c->sd > 0);
    ASSERT(evb->nchange < evb->nevent);

    if (!c->send_active) {
        return 0;
    }

    event = &evb->change[evb->nchange++];
    EV_SET(event, c->sd, EVFILT_WRITE, EV_DELETE, 0, 0, c);

    c->send_active = 0;

    /*
     * Eliminate subsequent WRITE events, if we have disabled WRITE during
     * processing one of the previous ones.
     */
    eliminate_pending_events(evb, c, EVFILT_WRITE);

    return 0;
}

int
event_add_conn(struct event_base *evb, struct conn *c)
{
    ASSERT(evb->kq > 0);
    ASSERT(c != NULL);
    ASSERT(c->sd > 0);
    ASSERT(!c->recv_active);
    ASSERT(!c->send_active);
    ASSERT(evb->nchange < evb->nevent);

    event_add_in(evb, c);
    event_add_out(evb, c);

    return 0;
}

int
event_del_conn(struct event_base *evb, struct conn *c)
{
    ASSERT(evb->kq > 0);
    ASSERT(c != NULL);
    ASSERT(c->sd > 0);
    ASSERT(evb->nchange < evb->nevent);

    event_del_out(evb, c);
    event_del_in(evb, c);

    /*
     * Now, eliminate all outstanding pending events for (c).
     * This is important because we will close and free (c) when we return.
     */
    eliminate_pending_events(evb, c, 0);

    return 0;
}

int
event_wait(struct event_base *evb, int timeout)
{
    int kq = evb->kq;
    struct timespec ts, *tsp;

    ASSERT(kq > 0);

    /* kevent should block indefinitely if timeout < 0 */
    if (timeout < 0) {
        tsp = NULL;
    } else {
        tsp = &ts;
        tsp->tv_sec = timeout / 1000LL;
        tsp->tv_nsec = (timeout % 1000LL) * 1000000LL;
    }

    for (;;) {
        /*
         * kevent() is used both to register new events with kqueue, and to
         * retrieve any pending events. Changes that should be applied to the
         * kqueue are given in the change[] and any returned events are placed
         * in event[], up to the maximum sized allowed by nevent. The number
         * of entries actually placed in event[] is returned by the kevent()
         * call and saved in nreturned.
         *
         * Events are registered with the system by the application via a
         * struct kevent, and an event is uniquely identified with the system
         * by a (kq, ident, filter) tuple. This means that there can be only
         * one (ident, filter) pair for a given kqueue.
         */
        evb->nreturned = kevent(kq, evb->change, evb->nchange, evb->event,
                                evb->nevent, tsp);
        evb->nchange = 0;
        if (evb->nreturned > 0) {
            for (evb->nprocessed = 0; evb->nprocessed < evb->nreturned;
                evb->nprocessed++) {
                struct kevent *ev = &evb->event[evb->nprocessed];
                uint32_t events = 0;

                log_debug(LOG_VVERB, "kevent %04"PRIX32" with filter %d "
                          "triggered on sd %d", ev->flags, ev->filter,
                          ev->ident);

                /*
                 * If an error occurs while processing an element of the
                 * change[] and there is enough room in the event[], then the
                 * event event will be placed in the eventlist with EV_ERROR
                 * set in flags and the system error(errno) in data.
                 */
                if (ev->flags & EV_ERROR) {
                   /*
                    * Error messages that can happen, when a delete fails.
                    *   EBADF happens when the file descriptor has been closed
                    *   ENOENT when the file descriptor was closed and then
                    *   reopened.
                    *   EINVAL for some reasons not understood; EINVAL
                    *   should not be returned ever; but FreeBSD does :-\
                    * An error is also indicated when a callback deletes an
                    * event we are still processing. In that case the data
                    * field is set to ENOENT.
                    */
                    if (ev->data == EBADF || ev->data == EINVAL ||
                        ev->data == ENOENT || ev->data == EINTR) {
                        continue;
                    }
                    events |= EVENT_ERR;
                }

                if (ev->filter == EVFILT_READ) {
                    events |= EVENT_READ;
                }

                if (ev->filter == EVFILT_WRITE) {
                    events |= EVENT_WRITE;
                }

                if (evb->cb != NULL && events != 0) {
                    struct conn *c = ev->udata;
                    evb->cb(ev->udata, events);
                    if((ev->flags & EV_EOF) && !c->done && !c->err) {
                        /* Invoke the callback again, if the fd is closed
                         * and we have not noticed it yet.
                         * Could not do it earlier because there might
                         * have been some data pending, so we had to
                         * simulate a clean WRITE (or READ?) first.
                         * Now we simulate READ again, it'll recv()=>0.
                         */
                        evb->cb(ev->udata, EVENT_READ);
                    }
                }
            }
            return evb->nreturned;
        }

        if (evb->nreturned == 0) {
            if (timeout == -1) {
               log_error("kevent on kq %d with %d events and %d timeout "
                         "returned no events", kq, evb->nevent, timeout);
                return -1;
            }

            return 0;
        }

        if (errno == EINTR) {
            continue;
        }

        log_error("kevent on kq %d with %d events failed: %s", kq, evb->nevent,
                  strerror(errno));
        return -1;
    }

    NOT_REACHED();
}

#endif /* NC_HAVE_KQUEUE */
