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

#ifndef _NC_HASHKIT_H_
#define _NC_HASHKIT_H_

#include <nc_core.h>
#include <nc_server.h>

#define HASH_CODEC(ACTION)                      \
    ACTION( HASH_ONE_AT_A_TIME, one_at_a_time ) \
    ACTION( HASH_MD5,           md5           ) \
    ACTION( HASH_CRC16,         crc16         ) \
    ACTION( HASH_CRC32,         crc32         ) \
    ACTION( HASH_FNV1_64,       fnv1_64       ) \
    ACTION( HASH_FNV1A_64,      fnv1a_64      ) \
    ACTION( HASH_FNV1_32,       fnv1_32       ) \
    ACTION( HASH_FNV1A_32,      fnv1a_32      ) \
    ACTION( HASH_HSIEH,         hsieh         ) \
    ACTION( HASH_MURMUR,        murmur        ) \
    ACTION( HASH_JENKINS,       jenkins       ) \

#define DIST_CODEC(ACTION)                      \
    ACTION( DIST_KETAMA,        ketama        ) \
    ACTION( DIST_MODULA,        modula        ) \
    ACTION( DIST_RANDOM,        random        ) \

#define DEFINE_ACTION(_hash, _name) _hash,
typedef enum hash_type {
    HASH_CODEC( DEFINE_ACTION )
    HASH_SENTINEL
} hash_type_t;
#undef DEFINE_ACTION

#define DEFINE_ACTION(_dist, _name) _dist,
typedef enum dist_type {
    DIST_CODEC( DEFINE_ACTION )
    DIST_SENTINEL
} dist_type_t;
#undef DEFINE_ACTION

uint32_t hash_one_at_a_time(const char *key, size_t key_length);
void md5_signature(const unsigned char *key, unsigned int length, unsigned char *result);
uint32_t hash_md5(const char *key, size_t key_length);
uint32_t hash_crc32(const char *key, size_t key_length);
uint32_t hash_crc16(const char *key, size_t key_length);
uint32_t hash_fnv1_64(const char *key, size_t key_length);
uint32_t hash_fnv1a_64(const char *key, size_t key_length);
uint32_t hash_fnv1_32(const char *key, size_t key_length);
uint32_t hash_fnv1a_32(const char *key, size_t key_length);
uint32_t hash_hsieh(const char *key, size_t key_length);
uint32_t hash_jenkins(const char *key, size_t length);
uint32_t hash_murmur(const char *key, size_t length);

rstatus_t ketama_update(struct server_pool *pool);
uint32_t ketama_dispatch(struct continuum *continuum, uint32_t ncontinuum, uint32_t hash);
rstatus_t modula_update(struct server_pool *pool);
uint32_t modula_dispatch(struct continuum *continuum, uint32_t ncontinuum, uint32_t hash);
rstatus_t random_update(struct server_pool *pool);
uint32_t random_dispatch(struct continuum *continuum, uint32_t ncontinuum, uint32_t hash);

#endif
