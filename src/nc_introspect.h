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

#ifndef _NC_INTROSPECT_H_
#define _NC_INTROSPECT_H_

#include <nc_core.h>

/*
 * Generate and return the buffer representing the current layout
 * of proxies, servers, connections.
 * Freeing the resulting buffer using nc_free is caller's responsibility.
 */
enum fro_format_details {
    FRO_POOLS        = 0x00, /* Does not make sense to not to print pools */
    FRO_SERVERS      = 0x01,
    FRO_SERVER_CONNS = 0x03, /* FRO_SERVERS implied */
    FRO_CLIENT_CONNS = 0x04, /* Can be lots of them... */
    /* Detail */
    FRO_DETAIL_DEFAULT = 0x000,
    FRO_DETAIL_VERBOSE = 0x100,
};
char *format_runtime_objects(struct context *ctx, struct server_pools *, enum fro_format_details);

void log_runtime_objects(int level, struct context *ctx, struct server_pools *, enum fro_format_details);

#endif
