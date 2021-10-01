/*
 * twemproxy - A fast and lightweight proxy for memcached protocol.
 * 
 * Copyright (C) 2021, wei huang <wei.kukey@gmail.com>
 * All rights reserved.
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

#ifndef NC_MONITOR_H
#define NC_MONITOR_H

#include <stdbool.h>
#include <nc_core.h>

#define CONF_DEFAULT_ARRAY_MONITOR_NUM       2

void monitor_init(struct server_pool *sp);
void monitor_deinit(struct server_pool *sp);

inline bool monitor_is_empty(struct server_pool *sp) {
    return array_n(&sp->monitor_conns) == 0;
}

rstatus_t add_to_monitor(struct conn *c);
void del_from_monitor(struct conn *c);
rstatus_t rsp_send_monitor_msg(struct context *ctx, struct conn *c, struct msg *m);

#endif