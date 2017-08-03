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

#ifndef _NC_HASH_H_
#define _NC_HASH_H_

#include <nc_core.h>

typedef int (*hash_compare_t)(const void *, uint32_t, const void *, uint32_t);
typedef rstatus_t (*hash_each_t)(void *, uint32_t, void *, uint32_t, void *);

struct hash_entry {
    void *key;
    uint32_t ksize;
    void *value;
    uint32_t vsize;
    struct hash_entry *next;
} hash_entry;

struct hash {
    struct hash_entry **buckets;
    uint32_t size;
    uint32_t used;
    bool nocase;
};

static inline uint32_t
hash_count(const struct hash *h)
{
    return h->size;
}

struct hash *hash_create(uint32_t buckets, bool nocase);
void hash_destroy(struct hash *h);
rstatus_t hash_init(struct hash *h, uint32_t buckets, bool nocase);
void hash_deinit(struct hash *h);

void hash_set(struct hash *h, const void *key, uint32_t ksize, const void *value, uint32_t vsize);
rstatus_t hash_remove(struct hash *h, const void *key, uint32_t ksize);
rstatus_t hash_find(struct hash *h, const void *key, uint32_t ksize, const void **value, uint32_t *vsize);
void hash_clear(struct hash *h);
rstatus_t hash_each(struct hash *a, hash_each_t func, void *data);

#endif
