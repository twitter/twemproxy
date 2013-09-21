/*
 * twemproxy - A fast and lightweight proxy for memcached protocol.
 * Copyright (C) 2013 Twitter, Inc.
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

#ifdef NC_HAVE_EVENT_PORTS

#include <port.h>
#include <poll.h>

struct event_base *
event_base_create(int nevent, event_cb_t cb)
{
    struct event_base *evb;
    int status, evp;
    port_event_t *event;

    ASSERT(nevent > 0);

    evp = port_create();
    if (evp < 0) {
        log_error("port create failed: %s", strerror(errno));
        return NULL;
    }

    event = nc_calloc(nevent, sizeof(*event));
    if (event == NULL) {
        status = close(evp);
        if (status < 0) {
            log_error("close evp %d failed, ignored: %s", evp, strerror(errno));
        }
        return NULL;
    }

    evb = nc_alloc(sizeof(*evb));
    if (evb == NULL) {
        nc_free(event);
        status = close(evp);
        if (status < 0) {
            log_error("close evp %d failed, ignored: %s", evp, strerror(errno));
        }
        return NULL;
    }

    evb->evp = evp;
    evb->event = event;
    evb->nevent = nevent;
    evb->cb = cb;

    log_debug(LOG_INFO, "evp %d with nevent %d", evb->evp, evb->nevent);

    return evb;
}

void
event_base_destroy(struct event_base *evb)
{
    int status;

    if (evb == NULL) {
        return;
    }

    ASSERT(evb->evp >= 0);

    nc_free(evb->event);

    status = close(evb->evp);
    if (status < 0) {
        log_error("close evp %d failed, ignored: %s", evb->evp, strerror(errno));
    }
    evb->evp = -1;

    nc_free(evb);
}

int
event_add_in(struct event_base *evb, struct conn *c)
{
    return 0;
}

int
event_del_in(struct event_base *evb, struct conn *c)
{
    return 0;
}

int
event_add_out(struct event_base *evb, struct conn *c)
{
    int status;
    int evp = evb->evp;

    ASSERT(evp > 0);
    ASSERT(c != NULL);
    ASSERT(c->sd > 0);
    ASSERT(c->recv_active);

    if (c->send_active) {
        return 0;
    }

    status = port_associate(evp, PORT_SOURCE_FD, c->sd, POLLIN | POLLOUT, c);
    if (status < 0) {
        log_error("port associate on evp %d sd %d failed: %s", evp, c->sd,
                  strerror(errno));
    } else {
        c->send_active = 1;
    }

    return status;
}

int
event_del_out(struct event_base *evb, struct conn *c)
{
    int status;
    int evp = evb->evp;

    ASSERT(evp > 0);
    ASSERT(c != NULL);
    ASSERT(c->sd > 0);
    ASSERT(c->recv_active);

    if (!c->send_active) {
        return 0;
    }

    status = port_associate(evp, PORT_SOURCE_FD, c->sd, POLLIN, c);
    if (status < 0) {
        log_error("port associate on evp %d sd %d failed: %s", evp, c->sd,
                  strerror(errno));
    } else {
        c->send_active = 0;
    }

    return status;
}

int
event_add_conn(struct event_base *evb, struct conn *c)
{
    int status;
    int evp = evb->evp;

    ASSERT(evp > 0);
    ASSERT(c != NULL);
    ASSERT(c->sd > 0);
    ASSERT(!c->recv_active);
    ASSERT(!c->send_active);

    status = port_associate(evp, PORT_SOURCE_FD, c->sd, POLLIN | POLLOUT, c);
    if (status < 0) {
        log_error("port associate on evp %d sd %d failed: %s", evp, c->sd,
                  strerror(errno));
    } else {
        c->send_active = 1;
        c->recv_active = 1;
    }

    return status;
}

int
event_del_conn(struct event_base *evb, struct conn *c)
{
    int status;
    int evp = evb->evp;

    ASSERT(evp > 0);
    ASSERT(c != NULL);
    ASSERT(c->sd > 0);

    if (!c->send_active && !c->recv_active) {
        return 0;
    }

    /*
     * Removes the association of an object with a port. The association
     * is also removed if the port gets closed.
     *
     * On failure, we check for ENOENT errno because it is likely that we
     * are deleting this connection after it was returned from the event
     * loop and before we had a chance of reactivating it by calling
     * port_associate() on it.
     */
    status = port_dissociate(evp, PORT_SOURCE_FD, c->sd);
    if (status < 0 && errno != ENOENT) {
        log_error("port dissociate evp %d sd %d failed: %s", evp, c->sd,
                  strerror(errno));
        return status;
    }

    c->recv_active = 0;
    c->send_active = 0;

    return 0;
}

static int
event_reassociate(struct event_base *evb, struct conn *c)
{
    int status, events;
    int evp = evb->evp;

    ASSERT(evp > 0);
    ASSERT(c != NULL);
    ASSERT(c->sd > 0);
    ASSERT(c->recv_active);

    if (c->send_active) {
        events = POLLIN | POLLOUT;
    } else {
        events = POLLIN;
    }

    status = port_associate(evp, PORT_SOURCE_FD, c->sd, events , c);
    if (status < 0) {
        log_error("port associate on evp %d sd %d failed: %s", evp, c->sd,
                  strerror(errno));
    }

    return status;
}

int
event_wait(struct event_base *evb, int timeout)
{
    int evp = evb->evp;
    port_event_t *event = evb->event;
    int nevent = evb->nevent;
    struct timespec ts, *tsp;

    ASSERT(evp > 0);
    ASSERT(event != NULL);
    ASSERT(nevent > 0);

    /* port_getn should block indefinitely if timeout < 0 */
    if (timeout < 0) {
        tsp = NULL;
    } else {
        tsp = &ts;
        tsp->tv_sec = timeout / 1000LL;
        tsp->tv_nsec = (timeout % 1000LL) * 1000000LL;
    }

    for (;;) {
        int i, status;
        unsigned int nreturned = 1;

        /*
         * port_getn() retrieves multiple events from a port. A port_getn()
         * call will block until at least nreturned events is triggered. On
         * a successful return event[] is populated with triggered events
         * up to the maximum sized allowed by nevent. The number of entries
         * actually placed in event[] is saved in nreturned, which may be
         * more than what we asked for but less than nevent.
         */
        status = port_getn(evp, event, nevent, &nreturned, tsp);
        if (status < 0) {
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            }

            /*
             * ETIME - The time interval expired before the expected number
             * of events have been posted to the port or nreturned is updated
             * with the number of returned port_event_t structures in event[]
             */
            if (errno != ETIME) {
                log_error("port getn on evp %d with %d events failed: %s", evp,
                          nevent, strerror(errno));
                return -1;
            }
        }

        if (nreturned > 0) {
            for (i = 0; i < nreturned; i++) {
                port_event_t *ev = &evb->event[i];
                uint32_t events = 0;

                log_debug(LOG_VVERB, "port %04"PRIX32" from source %d "
                          "triggered on conn %p", ev->portev_events,
                          ev->portev_source, ev->portev_user);

                if (ev->portev_events & POLLERR) {
                    events |= EVENT_ERR;
                }

                if (ev->portev_events & POLLIN) {
                    events |= EVENT_READ;
                }

                if (ev->portev_events & POLLOUT) {
                    events |= EVENT_WRITE;
                }

                if (evb->cb != NULL && events != 0) {
                    status = evb->cb(ev->portev_user, events);
                    if (status < 0) {
                        continue;
                    }

                    /*
                     * When an event for a PORT_SOURCE_FD object is retrieved,
                     * the object no longer has an association with the port.
                     * The event can be processed without the possibility that
                     * another thread can retrieve a subsequent event for the
                     * same object. After processing of the file descriptor
                     * is completed, the port_associate() function can be
                     * called to reassociate the object with the port.
                     *
                     * If the descriptor is still capable of accepting data,
                     * this reassociation is required for the reactivation of
                     * the data detection.
                     */
                    event_reassociate(evb, ev->portev_user);
                }
            }

            return nreturned;
        }

        if (timeout == -1) {
            log_error("port getn on evp %d with %d events and %d timeout "
                      "returned no events", evp, nevent, timeout);
            return -1;
        }

        return 0;
    }

    NOT_REACHED();
}

void
event_loop_stats(event_stats_cb_t cb, void *arg)
{
    struct stats *st = arg;
    int status, evp;
    port_event_t event;
    struct timespec ts, *tsp;

    evp = port_create();
    if (evp < 0) {
        log_error("port create failed: %s", strerror(errno));
        return;
    }

    status = port_associate(evp, PORT_SOURCE_FD, st->sd, POLLIN, NULL);
    if (status < 0) {
        log_error("port associate on evp %d sd %d failed: %s", evp, st->sd,
                  strerror(errno));
        goto error;
    }

    /* port_getn should block indefinitely if st->interval < 0 */
    if (st->interval < 0) {
        tsp = NULL;
    } else {
        tsp = &ts;
        tsp->tv_sec = st->interval / 1000LL;
        tsp->tv_nsec = (st->interval % 1000LL) * 1000000LL;
    }


    for (;;) {
        unsigned int nreturned = 1;

        status = port_getn(evp, &event, 1, &nreturned, tsp);
        if (status != NC_OK) {
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            }

            if (errno != ETIME) {
                log_error("port getn on evp %d with m %d failed: %s", evp,
                          st->sd, strerror(errno));
                goto error;
            }
        }

        ASSERT(nreturned <= 1);

        if (nreturned == 1) {
            /* re-associate monitoring descriptor with the port */
            status = port_associate(evp, PORT_SOURCE_FD, st->sd, POLLIN, NULL);
            if (status < 0) {
                log_error("port associate on evp %d sd %d failed: %s", evp, st->sd,
                          strerror(errno));
            }
        }

        cb(st, &nreturned);
    }

error:
    status = close(evp);
    if (status < 0) {
        log_error("close evp %d failed, ignored: %s", evp, strerror(errno));
    }
    evp = -1;
}

#endif /* NC_HAVE_EVENT_PORTS */
