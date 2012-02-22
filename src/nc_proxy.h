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

#ifndef _NC_PROXY_H_
#define _NC_PROXY_H_

#include <nc_core.h>

void proxy_ref(struct conn *conn, void *owner);
void proxy_unref(struct conn *conn);
void proxy_close(struct context *ctx, struct conn *conn);

rstatus_t proxy_each_init(void *elem, void *data);
rstatus_t proxy_each_deinit(void *elem, void *data);

rstatus_t proxy_init(struct context *ctx);
void proxy_deinit(struct context *ctx);
rstatus_t proxy_recv(struct context *ctx, struct conn *conn);

#endif
