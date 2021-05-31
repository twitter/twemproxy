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

#ifndef _NC_SIGNAL_CONN_H_
#define _NC_SIGNAL_CONN_H_

#include <signal.h>
#include <nc_core.h>

struct conn *create_signal_listener(struct context *ctx);

void conn_signal_ref(struct conn *conn, void *owner);
void conn_signal_unref(struct conn *conn);
rstatus_t conn_signal_recv(struct context *ctx, struct conn *connconn);
void conn_signal_close(struct context *ctx, struct conn *conn);

/*
 * Return the next signal that has been received.
 * Returns 0 if no signals have been received.
 */
int conn_next_signal(void);

#endif
