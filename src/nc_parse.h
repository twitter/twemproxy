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

#ifndef _NC_PARSE_H_
#define _NC_PARSE_H_

#include <nc_core.h>

/*
 * From memcache protocol specification:
 *
 * Data stored by memcached is identified with the help of a key. A key
 * is a text string which should uniquely identify the data for clients
 * that are interested in storing and retrieving it.  Currently the
 * length limit of a key is set at 250 characters (of course, normally
 * clients wouldn't need to use such long keys); the key must not include
 * control characters or whitespace.
 */
#define PARSE_MAX_KEY_LENGTH 250

typedef enum parse_result {
    PARSE_OK,                 /* parsing ok */
    PARSE_ERROR,              /* parsing error */
    PARSE_REPAIR,             /* more to parse -> repair parsed & unparsed data */
    PARSE_FRAGMENT,           /* multi-get request -> fragment */
    PARSE_AGAIN,              /* incomplete -> parse again */
} parse_result_t;

void parse_request(struct msg *r);
void parse_response(struct msg *r);

#endif
