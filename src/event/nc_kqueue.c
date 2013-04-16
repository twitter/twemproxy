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

struct evbase *
evbase_create(int nevent, void (*callback_fp)(void *, uint32_t))
{

    struct evbase *evb;
    int status, kq;
    struct kevent *changes, *kevents;

    if (nevent <= 0) {
        log_error("nevent has to be positive %d", nevent);
        return NULL;
    }

    /* Initialize the kernel queue */
    if ((kq = kqueue()) == -1) {
        log_error("kernel queue create failed: %s", kq, strerror(errno));
        return NULL;
    }

    changes = nc_calloc(nevent, sizeof(*changes));
    if (changes == NULL) {
        status = close(kq);
        if (status < 0) {
            log_error("close kq %d failed, ignored: %s", kq, strerror(errno));
        }
        return NULL;
    }

    kevents = nc_calloc(nevent, sizeof(*kevents));
    if (kevents == NULL) {
        nc_free(changes);
        status = close(kq);
        if (status < 0) {
            log_error("close kq %d failed, ignored: %s", kq, strerror(errno));
        }
        return NULL;
    }

    evb = (struct evbase *) nc_alloc(sizeof(*evb));
    if (evb == NULL) {
        nc_free(changes);
        nc_free(kevents);
        status = close(kq);
        if (status < 0) {
            log_error("close kq %d failed, ignored: %s", kq, strerror(errno));
        }
        return NULL;
    }

    evb->kq = kq;
    evb->changes = changes;
    evb->kevents  = kevents;
    evb->nevent = nevent;
    evb->callback_fp = callback_fp;
    evb->n_changes = 0;

    log_debug(LOG_INFO, "kq %d with nevent %d", evb->kq,
              evb->nevent);

    return evb;
}

void
evbase_destroy(struct evbase *evb)
{
    int status;

    if (evb == NULL) return;

    ASSERT(evb->kq >= 0);

    nc_free(evb->changes);
    nc_free(evb->kevents);

    status = close(evb->kq);
    if (status < 0) {
        log_error("close kq %d failed, ignored: %s", evb->kq, strerror(errno));
    }
    nc_free(evb);
}

int
event_add_out(struct evbase *evb, struct conn *c)
{
    struct kevent *event;
    int kq = evb->kq;

    ASSERT(kq > 0);
    ASSERT(c != NULL);
    ASSERT(c->sd > 0);
    ASSERT(c->recv_active);
    ASSERT(evb->n_changes < evb->nevent);

    if (c->send_active) {
        return 0;
    }

    event = &evb->changes[(evb->n_changes)++];
    EV_SET(event, c->sd, EVFILT_WRITE, EV_ADD | EV_CLEAR, 0, 0, (void *)c);

    c->send_active = 1;

    return 0;
}

int
event_del_out(struct evbase *evb, struct conn *c)
{
    struct kevent *event;
    int kq = evb->kq;

    ASSERT(kq > 0);
    ASSERT(c != NULL);
    ASSERT(c->sd > 0);
    ASSERT(c->recv_active);
    ASSERT(evb->n_changes < evb->nevent);

    if (!c->send_active) {
        return 0;
    }

    event = &evb->changes[(evb->n_changes)++];
    EV_SET(event, c->sd, EVFILT_WRITE, EV_DELETE, 0, 0, (void *)c);

    c->send_active = 0;

    return 0;
}

int
event_add_conn(struct evbase *evb, struct conn *c)
{
    struct kevent *event;
    int kq = evb->kq;

    ASSERT(kq > 0);
    ASSERT(c != NULL);
    ASSERT(c->sd > 0);
    ASSERT(evb->n_changes < evb->nevent);

    event = &evb->changes[(evb->n_changes)++];
    EV_SET(event, c->sd, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, (void *)c);

    c->recv_active = 1;

    event_add_out(evb, c);
    c->send_active = 1;

    return 0;
}

int
event_del_conn(struct evbase *evb, struct conn *c)
{
    int i;
    struct kevent *event;
    int kq = evb->kq;

    ASSERT(kq > 0);
    ASSERT(c != NULL);
    ASSERT(c->sd > 0);
    ASSERT(evb->n_changes < evb->nevent);

    event = &evb->changes[(evb->n_changes)++];
    EV_SET(event, c->sd, EVFILT_READ, EV_DELETE, 0, 0, (void *)c);

    event_del_out(evb, c);

    c->recv_active = 0;
    c->send_active = 0;

    /*
     * Now, eliminate pending events for c->sd (there should be at most one
     * other event). This is important because we will close c->sd and free
     * c when we return.
     */
    for (i = evb->n_processed + 1; i < evb->n_returned; i++) {
        struct kevent *ev = &evb->kevents[i];
        if (ev->ident == (uintptr_t)c->sd) {
            ev->flags = 0;
            ev->filter = 0;
            break;
        }
    }

    return 0;
}

int
event_wait(struct evbase *evb, int timeout)
{
    int kq = evb->kq;
    struct timespec ts = nc_millisec_to_timespec(timeout);
    void (*callback_fp)(void *, uint32_t) = evb->callback_fp;

    ASSERT(kq > 0);

    for (;;) {
        evb->n_returned = kevent(kq, evb->changes, evb->n_changes, evb->kevents,
                                 evb->nevent, &ts);
        evb->n_changes = 0;
        if (evb->n_returned > 0) {
            for (evb->n_processed = 0; evb->n_processed < evb->n_returned;
                evb->n_processed++) {
                struct kevent *ev = &evb->kevents[evb->n_processed];
                uint32_t events = 0;

                if (ev->flags & EV_ERROR) {
                   /*
                    * Error messages that can happen, when a delete fails.
                    *   EBADF happens when the file descriptor has been
                    *   closed,
                    *   ENOENT when the file descriptor was closed and
                    *   then reopened.
                    *   EINVAL for some reasons not understood; EINVAL
                    *   should not be returned ever; but FreeBSD does :-\
                    * An error is also indicated when a callback deletes
                    * an event we are still processing.  In that case
                    * the data field is set to ENOENT.
                    */
                    if (ev->data == EBADF || ev->data == EINVAL ||
                        ev->data == ENOENT) {
                        continue;
                    }
                    events |= EV_ERR;
                }

                if (ev->filter == EVFILT_READ) {
                    events |= EV_READ;
                }

                if (ev->filter == EVFILT_WRITE) {
                    events |= EV_WRITE;
                }

                if (callback_fp != NULL && events != 0) {
                    (*callback_fp)((void *)(ev->udata), events);
                }
            }
            return evb->n_returned;
        }

        if (evb->n_returned == 0) {
            if (timeout == -1) {
               log_error("kqueue on kq %d with %d events and %d timeout "
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

int
event_add_st(struct evbase *evb, int fd)
{
    struct kevent *ev = &evb->changes[(evb->n_changes)++];

    EV_SET(ev, fd, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, NULL);

    return 0;
}

#endif /* NC_HAVE_KQUEUE */
