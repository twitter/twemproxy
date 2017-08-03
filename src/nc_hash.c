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

#include <stdlib.h>

#include <nc_core.h>
#include <hashkit/nc_hashkit.h>

struct hash *hash_create(uint32_t buckets, bool nocase)
{
    struct hash *h = nc_alloc(sizeof(struct hash));
    if (h != NULL && hash_init(h, buckets, nocase) != NC_OK) {
        nc_free(h);
        h = NULL;
    }
    return h;
}

void hash_destroy(struct hash *h)
{
    hash_deinit(h);
    nc_free(h);
}

rstatus_t hash_init(struct hash *h, uint32_t buckets, bool nocase)
{
    h->size = buckets;
    h->used = 0;
    h->nocase = nocase;
    if ((h->buckets = nc_zalloc(sizeof(struct hash *) * buckets)) == NULL) {
        return NC_ENOMEM;
    }
    return NC_OK;
}

void hash_deinit(struct hash *h)
{
    hash_clear(h);
    nc_free(h->buckets);
}

typedef int32_t (*hash_comparator)(const void *, const void *, uint32_t);

static int32_t hash_compare_nocase(const void *l, const void *r, uint32_t size)
{
  return strncasecmp(l, r, size);
}

static int32_t hash_compare(const void *l, const void *r, uint32_t size)
{
  return memcmp(l, r, size);
}

static inline uint32_t hash_index(struct hash *h, const void *key, uint32_t ksize)
{
    if (h->nocase) {
        uint32_t index;
        char buffer[64];
        char *lowercase = buffer;
        if (ksize > sizeof(buffer)) {
            lowercase = nc_alloc(ksize);
        }
        for (index = 0; index < ksize; ++index) {
            lowercase[index] = (char)(tolower(((const char *)key)[index]));
        }
        index = hash_fnv1a_64(lowercase, ksize) % h->size;
        if (lowercase != buffer) {
            nc_free(lowercase);
        }
        return index;
    } else {
        return hash_fnv1a_64(key, ksize) % h->size;
    }
}

static inline struct hash_entry *hash_find_entry(struct hash *h, const void *key, uint32_t ksize)
{
    struct hash_entry *entry;
    uint32_t idx;
    hash_comparator comparator;
    if (h->used == 0) {
        return NULL;
    }
    comparator = h->nocase ? hash_compare_nocase : hash_compare;
    idx = hash_index(h, key, ksize);
    entry = h->buckets[idx];
    while (entry) {
        if (ksize == entry->ksize && comparator(key, entry->key, ksize) == 0) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

static inline void hash_entry_free(struct hash_entry *entry)
{
    nc_free(entry->key);
    nc_free(entry->value);
    nc_free(entry);
}

static inline void *hash_dup(const void *data, uint32_t size)
{
    void *copy = nc_alloc(size);
    memcpy(copy, data, size);
    return copy;
}

void hash_set(struct hash *h, const void *key, uint32_t ksize, const void *value, uint32_t vsize)
{
    uint32_t index;
    struct hash_entry *entry;
    if ((entry = hash_find_entry(h, key, ksize)) == NULL) {
        entry = nc_alloc(sizeof(struct hash_entry));
        entry->key = hash_dup(key, ksize);
        entry->ksize = ksize;
        entry->value = hash_dup(value, vsize);
        entry->vsize = vsize;
        index = hash_index(h, key, ksize);
        entry->next = h->buckets[index];
        h->buckets[index] = entry;
        h->used++;
    } else {
        nc_free(entry->value);
        entry->value = hash_dup(value, vsize);
        entry->vsize = vsize;
    }
}

rstatus_t hash_remove(struct hash *h, const void *key, uint32_t ksize)
{
    uint32_t index;
    struct hash_entry *entry, *prev;
    hash_comparator comparator;
    if (h->used == 0) {
        return NC_ERROR;
    }
    comparator = h->nocase ? hash_compare_nocase : hash_compare;
    index = hash_index(h, key, ksize);
    entry = h->buckets[index];
    prev = NULL;
    while (entry) {
        if (ksize == entry->ksize && comparator(key, entry->key, ksize) == 0) {
            if (prev) {
                prev->next = entry->next;
            } else {
                h->buckets[index] = entry->next;
            }
            hash_entry_free(entry);
            h->used--;
            return NC_OK;
        }
        prev = entry;
        entry = entry->next;
    }
  
    return NC_ERROR;
}

rstatus_t hash_find(struct hash *h, const void *key, uint32_t ksize, const void **value, uint32_t *vsize)
{
    struct hash_entry *entry;
    if ((entry = hash_find_entry(h, key, ksize)) == NULL) {
        return NC_ERROR;
    }
    if (value != NULL) {
        *value = entry->value;
    }
    if (vsize != NULL) {
        *vsize = entry->vsize;
    }
    return NC_OK;
}

void hash_clear(struct hash *h)
{
    uint32_t idx, left = h->used;
    struct hash_entry *entry = NULL;
    struct hash_entry *next = NULL;
  
    for (idx = 0; idx < h->size && left > 0; ++idx) {
        if ((entry = h->buckets[idx]) == NULL) {
            continue;
        }
        while (entry) {
            next = entry->next;
            hash_entry_free(entry);
            --left;
            entry = next;
        }
        h->buckets[idx] = NULL;
    }

    h->used = 0;
}

rstatus_t hash_each(struct hash *h, hash_each_t func, void *data)
{
    rstatus_t status;
    uint32_t idx, left = h->used;
    struct hash_entry *entry = NULL;
  
    for (idx = 0; idx < h->size && left > 0; ++idx) {
        if ((entry = h->buckets[idx]) == NULL) {
            continue;
        }
        while (entry) {
            if ((status = func(entry->key, entry->ksize, entry->value, entry->vsize, data)) != NC_OK) {
                return status;
            }
            --left;
            entry = entry->next;
        }
    }
    return NC_OK;
}
