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

#ifndef NC_CLIENT_H_AC3C4D0E44AC4FBD83D0E9B747BE23FD
#define NC_CLIENT_H_AC3C4D0E44AC4FBD83D0E9B747BE23FD

#include <nc_core.h>

bool client_active(struct conn *conn);
void client_ref(struct conn *conn, void *owner);
void client_unref(struct conn *conn);
void client_close(struct context *ctx, struct conn *conn);

#endif
