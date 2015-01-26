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

#ifdef NC_HAVE_POLL

#include <poll.h>
#include <nc_event.h>

#define INVALID_FD 0 
#define INVALID_INDEX -1 


struct event_base *
event_base_create(int nevent, event_cb_t cb)
{
    struct event_base *evb;
    struct pollfd *poll_event;

    ASSERT(npoll_event > 0);

    poll_event = nc_calloc(nevent, sizeof(*poll_event));
    if (poll_event == NULL) {
        log_error("poll_event create error ignored: %s",  strerror(errno));
        return NULL;
    }

    evb = nc_alloc(sizeof(*evb));
    if (evb == NULL) {
        nc_free(poll_event);
        log_error("evb create error ignored: %s",  strerror(errno));
        return NULL;
    }

    evb->poll_event = poll_event;
    evb->nevent = nevent;
    evb->cb = cb;

    evb->fu_array = nc_calloc(nevent, sizeof(*evb->fu_array)); 

    log_debug(LOG_INFO, "poll with npoll_event %d", evb->nevent);

    return evb;
}

void
event_base_destroy(struct event_base *evb)
{

    if (evb == NULL) {
        return;
    }


    nc_free(evb->poll_event);

    nc_free(evb->fu_array);

    nc_free(evb);
}

int
event_add_in(struct event_base *evb, struct conn *c)
{
    struct pollfd* poll_event = evb->poll_event;
    struct fd_unit* fu_array = evb->fu_array;

    ASSERT(c != NULL);
    ASSERT(c->sd > 0);

    if (c->recv_active) {
        return 0;
    }

    int index = fu_array[c->sd].idx;
    ASSERT(poll_event[index]->fd == c->sd);

    
    (poll_event+index)->events = POLLIN;
    fu_array[c->sd].cn = c;
    c->recv_active = 1;

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
    struct pollfd* poll_event = evb->poll_event;
    struct fd_unit* fu_array = evb->fu_array;

    ASSERT(c != NULL);
    ASSERT(c->sd > 0);
    ASSERT(c->recv_active);

    if (c->send_active) {
        return 0;
    }

    int index = fu_array[c->sd].idx;
    ASSERT(poll_event[index]->fd == c->sd);
    fu_array[c->sd].cn = c;

    
    (poll_event+index)->events = POLLIN|POLLOUT;
    c->send_active = 1;

    return 0;
}

int
event_del_out(struct event_base *evb, struct conn *c)
{
    struct pollfd* poll_event = evb->poll_event;
    struct fd_unit* fu_array = evb->fu_array;

    ASSERT(c != NULL);
    ASSERT(c->sd > 0);
    ASSERT(c->recv_active);

    if (!c->send_active) {
        return 0;
    }

    int index = fu_array[c->sd].idx;
    ASSERT((poll_event+index)->fd == c->sd);
    fu_array[c->sd].cn = c;

    
    (poll_event+index)->events = POLLIN;
    c->send_active = 0;

    return 0;
}

int
event_add_conn(struct event_base *evb, struct conn *c)
{
    struct pollfd* poll_event = evb->poll_event;
    struct fd_unit* fu_array = evb->fu_array;

    ASSERT(c != NULL);
    ASSERT(c->sd > 0);

    int i = 0;
    for(; i < evb->nevent; i++)
    {
        if((poll_event+i)->fd == INVALID_FD)
        {
            break;
        }
    }

    if(i >= evb->nevent)
    {
        log_error("addconn sd %d failed: %s",  c->sd,
                  strerror(errno));
        return -1;
    } 

    (poll_event+i)->fd = c->sd; 
    (poll_event+i)->events = POLLIN|POLLOUT;

    ASSERT(fu_array[c->sd].idx == INVALID_INDEX);
    ASSERT(fu_array[c->sd].cn == 0);

    fu_array[c->sd].idx = i;
    fu_array[c->sd].cn = c;

    c->recv_active = 1;
    c->send_active = 1;

    return 0;
}

int
event_del_conn(struct event_base *evb, struct conn *c)
{
    struct pollfd* poll_event = evb->poll_event;
    struct fd_unit* fu_array = evb->fu_array;

    ASSERT(c != NULL);
    ASSERT(c->sd > 0);

    int index = fu_array[c->sd].idx;
    ASSERT(poll_event[index]->fd == c->sd);

    (poll_event+index)->fd = INVALID_FD;
    (poll_event+index)->events = 0;
    fu_array[c->sd].idx = INVALID_INDEX;
    fu_array[c->sd].cn = 0;

    c->recv_active = 0;
    c->send_active = 0;

    return 0;
}

int
event_wait(struct event_base *evb, int timeout)
{
    struct pollfd* poll_event = evb->poll_event;
    struct fd_unit* fu_array = evb->fu_array;
    int nevent = evb->nevent;

    ASSERT(poll_event != NULL);
    ASSERT(npoll_event > 0);

    for (;;) {
        int i;
        int nsd = poll(poll_event, nevent, timeout);
        if (nsd > 0) {
            int cnt = 0;
            for (i = 0; i < nevent; i++) {
                if(cnt >= nsd)
                {
                    break;
                }
                struct pollfd* ev = poll_event+i;

                if(ev->revents == 0)
                {
                    continue;
                }

                cnt++;
                uint32_t poll_events = 0;
                struct conn* c = fu_array[ev->fd].cn;
                ASSERT(c);
                log_debug(LOG_VVERB, "poll %04"PRIX32" triggered on conn %p",
                          ev->revents, c);

                if (ev->revents & POLLERR) {
                    poll_events |= EVENT_ERR;
                }

                if (ev->revents & (POLLIN | POLLHUP)) {
                    poll_events |= EVENT_READ;
                }

                if (ev->revents & POLLOUT) {
                    poll_events |= EVENT_WRITE;
                }

                if (evb->cb != NULL) {
                    evb->cb(c, poll_events);
                }
            }
            return nsd;
        }

        if (nsd == 0) {
            if (timeout == -1) {
               log_error("poll with %d poll_events and %d timeout "
                         "returned no poll_events", nevent, timeout);
                return -1;
            }

            return 0;
        }

        if (errno == EINTR) {
            continue;
        }

        log_error("poll on with %d poll_events failed: %s", nevent, strerror(errno));
        return -1;
    }

    NOT_REACHED();
}

void
event_loop_stats(event_stats_cb_t cb, void *arg)
{
    struct stats *st = arg;
    struct pollfd poll_event;

    poll_event.fd = st->sd;
    poll_event.events = POLLIN;


    for (;;) {
        int n;

        n = poll(&poll_event, 1, st->interval);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            log_error("poll on with m %d failed: %s", 
                      st->sd, strerror(errno));
            break;
        }

        cb(st, &n);
    }

}

#endif /* NC_HAVE_POLL */

