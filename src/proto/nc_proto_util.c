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

#include <stdio.h>
#include <ctype.h>

#include <nc_core.h>
#include <nc_proto.h>

#define MINIMUM_SUB_MSGS_SIZE 4096

struct msg **get_sub_msgs(uint32_t ncontinuum) {
    static uint32_t min_size = MINIMUM_SUB_MSGS_SIZE;
    static struct msg *origin[MINIMUM_SUB_MSGS_SIZE];
    static struct msg **sub_msgs = origin;

    if (ncontinuum > min_size) {
        if (sub_msgs == origin) {
            sub_msgs = nc_alloc(ncontinuum * sizeof(struct msg *));
        } else {
            sub_msgs = nc_realloc(sub_msgs, ncontinuum * sizeof(struct msg *));
        }
        min_size = ncontinuum;
    }

    if (sub_msgs) {
        memset(sub_msgs, 0, ncontinuum * sizeof(struct msg *));
    }

    return sub_msgs;
}
