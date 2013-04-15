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

#ifndef _NC_CONF_H_
#define _NC_CONF_H_

#include <unistd.h>
#include <sys/types.h>
#include <sys/un.h>
#include <yaml.h>

#include <nc_core.h>
#include <hashkit/nc_hashkit.h>

#define CONF_OK             (void *) NULL
#define CONF_ERROR          (void *) "has an invalid value"

#define CONF_ROOT_DEPTH     1
#define CONF_MAX_DEPTH      CONF_ROOT_DEPTH + 1

#define CONF_DEFAULT_ARGS       3
#define CONF_DEFAULT_POOL       8
#define CONF_DEFAULT_SERVERS    8

#define CONF_UNSET_NUM  -1
#define CONF_UNSET_PTR  NULL
#define CONF_UNSET_HASH (hash_type_t) -1
#define CONF_UNSET_DIST (dist_type_t) -1

#define CONF_DEFAULT_HASH                    HASH_FNV1A_64
#define CONF_DEFAULT_DIST                    DIST_KETAMA
#define CONF_DEFAULT_TIMEOUT                 -1
#define CONF_DEFAULT_LISTEN_BACKLOG          512
#define CONF_DEFAULT_CLIENT_CONNECTIONS      0
#define CONF_DEFAULT_REDIS                   false
#define CONF_DEFAULT_PRECONNECT              false
#define CONF_DEFAULT_AUTO_EJECT_HOSTS        false
#define CONF_DEFAULT_SERVER_RETRY_TIMEOUT    30 * 1000      /* in msec */
#define CONF_DEFAULT_SERVER_FAILURE_LIMIT    2
#define CONF_DEFAULT_SERVER_CONNECTIONS      1
#define CONF_DEFAULT_KETAMA_PORT             11211

struct conf_listen {
    struct string   pname;   /* listen: as "name:port" */
    struct string   name;    /* name */
    int             port;    /* port */
    struct sockinfo info;    /* listen socket info */
    unsigned        valid:1; /* valid? */
};

struct conf_server {
    struct string   pname;      /* server: as "name:port:weight" */
    struct string   name;       /* name */
    int             port;       /* port */
    int             weight;     /* weight */
    struct sockinfo info;       /* connect socket info */
    unsigned        valid:1;    /* valid? */
};

struct conf_pool {
    struct string      name;                  /* pool name (root node) */
    struct conf_listen listen;                /* listen: */
    hash_type_t        hash;                  /* hash: */
    struct string      hash_tag;              /* hash_tag: */
    dist_type_t        distribution;          /* distribution: */
    int                timeout;               /* timeout: */
    int                backlog;               /* backlog: */
    int                client_connections;    /* client_connections: */
    int                redis;                 /* redis: */
    int                preconnect;            /* preconnect: */
    int                auto_eject_hosts;      /* auto_eject_hosts: */
    int                server_connections;    /* server_connections: */
    int                server_retry_timeout;  /* server_retry_timeout: in msec */
    int                server_failure_limit;  /* server_failure_limit: */
    struct array       server;                /* servers: conf_server[] */
    unsigned           valid:1;               /* valid? */
};

struct conf {
    char          *fname;           /* file name (ref in argv[]) */
    FILE          *fh;              /* file handle */
    struct array  arg;              /* string[] (parsed {key, value} pairs) */
    struct array  pool;             /* conf_pool[] (parsed pools) */
    uint32_t      depth;            /* parsed tree depth */
    yaml_parser_t parser;           /* yaml parser */
    yaml_event_t  event;            /* yaml event */
    yaml_token_t  token;            /* yaml token */
    unsigned      seq:1;            /* sequence? */
    unsigned      valid_parser:1;   /* valid parser? */
    unsigned      valid_event:1;    /* valid event? */
    unsigned      valid_token:1;    /* valid token? */
    unsigned      sound:1;          /* sound? */
    unsigned      parsed:1;         /* parsed? */
    unsigned      valid:1;          /* valid? */
};

struct command {
    struct string name;
    char          *(*set)(struct conf *cf, struct command *cmd, void *data);
    int           offset;
};

#define null_command { null_string, NULL, 0 }

char *conf_set_string(struct conf *cf, struct command *cmd, void *conf);
char *conf_set_listen(struct conf *cf, struct command *cmd, void *conf);
char *conf_add_server(struct conf *cf, struct command *cmd, void *conf);
char *conf_set_num(struct conf *cf, struct command *cmd, void *conf);
char *conf_set_bool(struct conf *cf, struct command *cmd, void *conf);
char *conf_set_hash(struct conf *cf, struct command *cmd, void *conf);
char *conf_set_distribution(struct conf *cf, struct command *cmd, void *conf);
char *conf_set_hashtag(struct conf *cf, struct command *cmd, void *conf);

rstatus_t conf_server_each_transform(void *elem, void *data);
rstatus_t conf_pool_each_transform(void *elem, void *data);

struct conf *conf_create(char *filename);
void conf_destroy(struct conf *cf);

#endif
