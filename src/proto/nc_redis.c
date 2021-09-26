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
#include <math.h>

#include <nc_core.h>
#include <nc_proto.h>

#define RSP_STRING(ACTION)                                                          \
    ACTION( ok,               "+OK\r\n"                                           ) \
    ACTION( pong,             "+PONG\r\n"                                         ) \
    ACTION( invalid_password, "-ERR invalid password\r\n"                         ) \
    ACTION( auth_required,    "-NOAUTH Authentication required\r\n"               ) \
    ACTION( no_password,      "-ERR Client sent AUTH, but no password is set\r\n" ) \

#define DEFINE_ACTION(_var, _str) static struct string rsp_##_var = string(_str);
    RSP_STRING( DEFINE_ACTION )
#undef DEFINE_ACTION

static rstatus_t redis_handle_auth_req(struct msg *request, struct msg *response);

/*
 * Return true, if the redis command take no key, otherwise
 * return false
 */
static bool
redis_argz(const struct msg *r)
{
    switch (r->type) {
    /* TODO: PING has an optional argument, emulate that? */
    case MSG_REQ_REDIS_PING:
    case MSG_REQ_REDIS_QUIT:
    case MSG_REQ_REDIS_COMMAND:
        return true;

    default:
        break;
    }

    return false;
}

/*
 * Return true, if the redis command accepts no arguments, otherwise
 * return false
 */
static bool
redis_arg0(const struct msg *r)
{
    switch (r->type) {
    case MSG_REQ_REDIS_PERSIST:
    case MSG_REQ_REDIS_PTTL:
    case MSG_REQ_REDIS_TTL:
    case MSG_REQ_REDIS_TYPE:
    case MSG_REQ_REDIS_DUMP:

    case MSG_REQ_REDIS_DECR:
    case MSG_REQ_REDIS_GET:
    case MSG_REQ_REDIS_GETDEL:
    case MSG_REQ_REDIS_INCR:
    case MSG_REQ_REDIS_STRLEN:

    case MSG_REQ_REDIS_HGETALL:
    case MSG_REQ_REDIS_HKEYS:
    case MSG_REQ_REDIS_HLEN:
    case MSG_REQ_REDIS_HVALS:

    case MSG_REQ_REDIS_LLEN:

    case MSG_REQ_REDIS_SCARD:
    case MSG_REQ_REDIS_SMEMBERS:

    case MSG_REQ_REDIS_ZCARD:
        /* TODO: Support emulating 2-arg username+password auth by just checking password? */
    case MSG_REQ_REDIS_AUTH:
        return true;

    default:
        break;
    }

    return false;
}

/*
 * Return true, if the redis command accepts exactly 1 argument, otherwise
 * return false
 */
static bool
redis_arg1(const struct msg *r)
{
    switch (r->type) {
    case MSG_REQ_REDIS_EXPIRE:
    case MSG_REQ_REDIS_EXPIREAT:
    case MSG_REQ_REDIS_PEXPIRE:
    case MSG_REQ_REDIS_PEXPIREAT:
    case MSG_REQ_REDIS_MOVE:

    case MSG_REQ_REDIS_APPEND:
    case MSG_REQ_REDIS_DECRBY:
    case MSG_REQ_REDIS_GETBIT:
    case MSG_REQ_REDIS_GETSET:
    case MSG_REQ_REDIS_INCRBY:
    case MSG_REQ_REDIS_INCRBYFLOAT:
    case MSG_REQ_REDIS_SETNX:

    case MSG_REQ_REDIS_HEXISTS:
    case MSG_REQ_REDIS_HGET:
    case MSG_REQ_REDIS_HSTRLEN:

    case MSG_REQ_REDIS_LINDEX:
    case MSG_REQ_REDIS_RPOPLPUSH:

    case MSG_REQ_REDIS_SISMEMBER:

    case MSG_REQ_REDIS_ZRANK:
    case MSG_REQ_REDIS_ZREVRANK:
    case MSG_REQ_REDIS_ZSCORE:
        return true;

    default:
        break;
    }

    return false;
}

/*
 * Return true, if the redis command accepts exactly 2 arguments, otherwise
 * return false
 */
static bool
redis_arg2(const struct msg *r)
{
    switch (r->type) {
    case MSG_REQ_REDIS_GETRANGE:
    case MSG_REQ_REDIS_PSETEX:
    case MSG_REQ_REDIS_SETBIT:
    case MSG_REQ_REDIS_SETEX:
    case MSG_REQ_REDIS_SETRANGE:

    case MSG_REQ_REDIS_HINCRBY:
    case MSG_REQ_REDIS_HINCRBYFLOAT:
    case MSG_REQ_REDIS_HSETNX:

    case MSG_REQ_REDIS_LRANGE:
    case MSG_REQ_REDIS_LREM:
    case MSG_REQ_REDIS_LSET:
    case MSG_REQ_REDIS_LTRIM:

    case MSG_REQ_REDIS_SMOVE:

    case MSG_REQ_REDIS_ZCOUNT:
    case MSG_REQ_REDIS_ZLEXCOUNT:
    case MSG_REQ_REDIS_ZINCRBY:
    case MSG_REQ_REDIS_ZREMRANGEBYLEX:
    case MSG_REQ_REDIS_ZREMRANGEBYRANK:
    case MSG_REQ_REDIS_ZREMRANGEBYSCORE:
        return true;

    default:
        break;
    }

    return false;
}

/*
 * Return true, if the redis command accepts exactly 3 arguments, otherwise
 * return false
 */
static bool
redis_arg3(const struct msg *r)
{
    switch (r->type) {
    case MSG_REQ_REDIS_LINSERT:
    case MSG_REQ_REDIS_LMOVE:
        return true;

    default:
        break;
    }

    return false;
}

/*
 * Return true, if the redis command operates on one key and accepts 0 or more arguments, otherwise
 * return false
 */
static bool
redis_argn(const struct msg *r)
{
    switch (r->type) {
    case MSG_REQ_REDIS_SORT:
    case MSG_REQ_REDIS_COPY:

    case MSG_REQ_REDIS_BITCOUNT:
    case MSG_REQ_REDIS_BITPOS:
    case MSG_REQ_REDIS_BITFIELD:
        /* TODO: Support REDIS_BITOP operation destkey key ... and add tests - this requires handling key in a position other than the first one */

    case MSG_REQ_REDIS_EXISTS:
    case MSG_REQ_REDIS_GETEX:
    case MSG_REQ_REDIS_SET:

    case MSG_REQ_REDIS_HDEL:
    case MSG_REQ_REDIS_HMGET:
    case MSG_REQ_REDIS_HMSET:
    case MSG_REQ_REDIS_HSCAN:
    case MSG_REQ_REDIS_HSET:
    case MSG_REQ_REDIS_HRANDFIELD:

    case MSG_REQ_REDIS_LPUSH:
    case MSG_REQ_REDIS_LPUSHX:
    case MSG_REQ_REDIS_RPUSH:
    case MSG_REQ_REDIS_RPUSHX:
    case MSG_REQ_REDIS_LPOP:
    case MSG_REQ_REDIS_RPOP:
    case MSG_REQ_REDIS_LPOS:

    case MSG_REQ_REDIS_SADD:
    case MSG_REQ_REDIS_SDIFF:
    case MSG_REQ_REDIS_SDIFFSTORE:
    case MSG_REQ_REDIS_SINTER:
    case MSG_REQ_REDIS_SINTERSTORE:
    case MSG_REQ_REDIS_SREM:
    case MSG_REQ_REDIS_SUNION:
    case MSG_REQ_REDIS_SUNIONSTORE:
    case MSG_REQ_REDIS_SRANDMEMBER:
    case MSG_REQ_REDIS_SSCAN:
    case MSG_REQ_REDIS_SPOP:
    case MSG_REQ_REDIS_SMISMEMBER:

    case MSG_REQ_REDIS_PFADD:
    case MSG_REQ_REDIS_PFMERGE:
    case MSG_REQ_REDIS_PFCOUNT:

    case MSG_REQ_REDIS_ZADD:
    case MSG_REQ_REDIS_ZDIFF:
    case MSG_REQ_REDIS_ZDIFFSTORE:
    case MSG_REQ_REDIS_ZINTER:
    case MSG_REQ_REDIS_ZINTERSTORE:
    case MSG_REQ_REDIS_ZMSCORE:
    case MSG_REQ_REDIS_ZPOPMAX:
    case MSG_REQ_REDIS_ZPOPMIN:
    case MSG_REQ_REDIS_ZRANDMEMBER:
    case MSG_REQ_REDIS_ZRANGE:
    case MSG_REQ_REDIS_ZRANGEBYLEX:
    case MSG_REQ_REDIS_ZRANGEBYSCORE:
    case MSG_REQ_REDIS_ZRANGESTORE:
    case MSG_REQ_REDIS_ZREM:
    case MSG_REQ_REDIS_ZREVRANGE:
    case MSG_REQ_REDIS_ZREVRANGEBYLEX:
    case MSG_REQ_REDIS_ZREVRANGEBYSCORE:
    case MSG_REQ_REDIS_ZSCAN:
    case MSG_REQ_REDIS_ZUNION:
    case MSG_REQ_REDIS_ZUNIONSTORE:

    case MSG_REQ_REDIS_GEODIST:
    case MSG_REQ_REDIS_GEOPOS:
    case MSG_REQ_REDIS_GEOHASH:
    case MSG_REQ_REDIS_GEOADD:
    case MSG_REQ_REDIS_GEORADIUS:
    case MSG_REQ_REDIS_GEORADIUSBYMEMBER:
    case MSG_REQ_REDIS_GEOSEARCH:
    case MSG_REQ_REDIS_GEOSEARCHSTORE:

    case MSG_REQ_REDIS_RESTORE:
        return true;

    default:
        break;
    }

    return false;
}

/*
 * Return true, if the redis command is a vector command accepting one or
 * more keys, otherwise return false
 */
static bool
redis_argx(const struct msg *r)
{
    switch (r->type) {
    case MSG_REQ_REDIS_MGET:
    case MSG_REQ_REDIS_DEL:
    case MSG_REQ_REDIS_UNLINK:
    case MSG_REQ_REDIS_TOUCH:
        return true;

    default:
        break;
    }

    return false;
}

/*
 * Return true, if the redis command is a vector command accepting one or
 * more key-value pairs, otherwise return false
 */
static bool
redis_argkvx(const struct msg *r)
{
    switch (r->type) {
    case MSG_REQ_REDIS_MSET:
        return true;

    default:
        break;
    }

    return false;
}

/*
 * Return true, if the redis command is either EVAL or EVALSHA. These commands
 * have a special format with exactly 2 arguments, followed by one or more keys,
 * followed by zero or more arguments (the documentation online seems to suggest
 * that at least one argument is required, but that shouldn't be the case).
 */
static bool
redis_argeval(const struct msg *r)
{
    switch (r->type) {
    case MSG_REQ_REDIS_EVAL:
    case MSG_REQ_REDIS_EVALSHA:
        return true;

    default:
        break;
    }

    return false;
}

static bool
redis_nokey(const struct msg *r)
{
    switch (r->type) {
    case MSG_REQ_REDIS_LOLWUT:
        return true;

    default:
        break;
    }

    return false;
}

/*
 * Return true, if the redis response is an error response i.e. a simple
 * string whose first character is '-', otherwise return false.
 */
static bool
redis_error(const struct msg *r)
{
    switch (r->type) {
    case MSG_RSP_REDIS_ERROR:
    case MSG_RSP_REDIS_ERROR_ERR:
    case MSG_RSP_REDIS_ERROR_OOM:
    case MSG_RSP_REDIS_ERROR_BUSY:
    case MSG_RSP_REDIS_ERROR_NOAUTH:
    case MSG_RSP_REDIS_ERROR_LOADING:
    case MSG_RSP_REDIS_ERROR_BUSYKEY:
    case MSG_RSP_REDIS_ERROR_MISCONF:
    case MSG_RSP_REDIS_ERROR_NOSCRIPT:
    case MSG_RSP_REDIS_ERROR_READONLY:
    case MSG_RSP_REDIS_ERROR_WRONGTYPE:
    case MSG_RSP_REDIS_ERROR_EXECABORT:
    case MSG_RSP_REDIS_ERROR_MASTERDOWN:
    case MSG_RSP_REDIS_ERROR_NOREPLICAS:
        return true;

    default:
        break;
    }

    return false;
}

/*
 * Reference: http://redis.io/topics/protocol
 *
 * Redis >= 1.2 uses the unified protocol to send requests to the Redis
 * server. In the unified protocol all the arguments sent to the server
 * are binary safe and every request has the following general form:
 *
 *   *<number of arguments> CR LF
 *   $<number of bytes of argument 1> CR LF
 *   <argument data> CR LF
 *   ...
 *   $<number of bytes of argument N> CR LF
 *   <argument data> CR LF
 *
 * Before the unified request protocol, redis protocol for requests supported
 * the following commands
 * 1). Inline commands: simple commands where arguments are just space
 *     separated strings. No binary safeness is possible.
 * 2). Bulk commands: bulk commands are exactly like inline commands, but
 *     the last argument is handled in a special way in order to allow for
 *     a binary-safe last argument.
 *
 * Nutcracker only supports the Redis unified protocol for requests.
 */
void
redis_parse_req(struct msg *r)
{
    struct mbuf *b;
    uint8_t *p, *m;
    uint8_t ch;
    enum {
        SW_START,
        SW_NARG,
        SW_NARG_LF,
        SW_REQ_TYPE_LEN,
        SW_REQ_TYPE_LEN_LF,
        SW_REQ_TYPE,
        SW_REQ_TYPE_LF,
        SW_KEY_LEN,
        SW_KEY_LEN_LF,
        SW_KEY,
        SW_KEY_LF,
        SW_ARG1_LEN,
        SW_ARG1_LEN_LF,
        SW_ARG1,
        SW_ARG1_LF,
        SW_ARG2_LEN,
        SW_ARG2_LEN_LF,
        SW_ARG2,
        SW_ARG2_LF,
        SW_ARG3_LEN,
        SW_ARG3_LEN_LF,
        SW_ARG3,
        SW_ARG3_LF,
        SW_ARGN_LEN,
        SW_ARGN_LEN_LF,
        SW_ARGN,
        SW_ARGN_LF,
        SW_SENTINEL
    } state;

    state = r->state;
    b = STAILQ_LAST(&r->mhdr, mbuf, next);

    ASSERT(r->request);
    ASSERT(state >= SW_START && state < SW_SENTINEL);
    ASSERT(b != NULL);
    ASSERT(b->pos <= b->last);

    /* validate the parsing maker */
    ASSERT(r->pos != NULL);
    ASSERT(r->pos >= b->pos && r->pos <= b->last);

    for (p = r->pos; p < b->last; p++) {
        ch = *p;

        switch (state) {

        case SW_START:
            ASSERT(r->token == NULL);
            if (ch != '*') {
                /* redis commands are always arrays */
                goto error;
            }
            r->token = p;
            /* req_start <- p */
            r->narg_start = p;
            r->rnarg = 0;
            state = SW_NARG;

            break;

        case SW_NARG:
            /* SW_NARG: The number of arguments in the redis command array */
            ASSERT(r->token != NULL);
            if (isdigit(ch)) {
                r->rnarg = r->rnarg * 10 + (uint32_t)(ch - '0');
            } else if (ch == CR) {
                if (r->rnarg == 0) {
                    goto error;
                }
                r->narg = r->rnarg;
                r->narg_end = p;
                r->token = NULL;
                state = SW_NARG_LF;
            } else {
                goto error;
            }

            break;

        case SW_NARG_LF:
            switch (ch) {
            case LF:
                state = SW_REQ_TYPE_LEN;
                break;

            default:
                goto error;
            }

            break;

        case SW_REQ_TYPE_LEN:
            if (r->token == NULL) {
                if (ch != '$') {
                    goto error;
                }
                r->token = p;
                r->rlen = 0;
            } else if (isdigit(ch)) {
                r->rlen = r->rlen * 10 + (uint32_t)(ch - '0');
            } else if (ch == CR) {
                if (r->rlen == 0 || r->rnarg == 0) {
                    goto error;
                }
                r->rnarg--;
                r->token = NULL;
                state = SW_REQ_TYPE_LEN_LF;
            } else {
                goto error;
            }

            break;

        case SW_REQ_TYPE_LEN_LF:
            switch (ch) {
            case LF:
                state = SW_REQ_TYPE;
                break;

            default:
                goto error;
            }

            break;

        case SW_REQ_TYPE:
            if (r->token == NULL) {
                r->token = p;
            }

            m = r->token + r->rlen;
            if (m >= b->last) {
                m = b->last - 1;
                p = m;
                break;
            }

            if (*m != CR) {
                goto error;
            }

            p = m; /* move forward by rlen bytes */
            r->rlen = 0;
            m = r->token;
            r->token = NULL;
            r->type = MSG_UNKNOWN;

            switch (p - m) {

            case 3:
                if (str3icmp(m, 'g', 'e', 't')) {
                    r->type = MSG_REQ_REDIS_GET;
                    break;
                }

                if (str3icmp(m, 's', 'e', 't')) {
                    r->type = MSG_REQ_REDIS_SET;
                    break;
                }

                if (str3icmp(m, 't', 't', 'l')) {
                    r->type = MSG_REQ_REDIS_TTL;
                    break;
                }

                if (str3icmp(m, 'd', 'e', 'l')) {
                    r->type = MSG_REQ_REDIS_DEL;
                    break;
                }

                break;

            case 4:
                if (str4icmp(m, 'p', 't', 't', 'l')) {
                    r->type = MSG_REQ_REDIS_PTTL;
                    break;
                }

                if (str4icmp(m, 'd', 'e', 'c', 'r')) {
                    r->type = MSG_REQ_REDIS_DECR;
                    break;
                }

                if (str4icmp(m, 'd', 'u', 'm', 'p')) {
                    r->type = MSG_REQ_REDIS_DUMP;
                    break;
                }

                if (str4icmp(m, 'h', 'd', 'e', 'l')) {
                    r->type = MSG_REQ_REDIS_HDEL;
                    break;
                }

                if (str4icmp(m, 'h', 'g', 'e', 't')) {
                    r->type = MSG_REQ_REDIS_HGET;
                    break;
                }

                if (str4icmp(m, 'h', 'l', 'e', 'n')) {
                    r->type = MSG_REQ_REDIS_HLEN;
                    break;
                }

                if (str4icmp(m, 'h', 's', 'e', 't')) {
                    r->type = MSG_REQ_REDIS_HSET;
                    break;
                }

                if (str4icmp(m, 'i', 'n', 'c', 'r')) {
                    r->type = MSG_REQ_REDIS_INCR;
                    break;
                }

                if (str4icmp(m, 'l', 'l', 'e', 'n')) {
                    r->type = MSG_REQ_REDIS_LLEN;
                    break;
                }

                if (str4icmp(m, 'l', 'p', 'o', 'p')) {
                    r->type = MSG_REQ_REDIS_LPOP;
                    break;
                }

                if (str4icmp(m, 'l', 'p', 'o', 's')) {
                    r->type = MSG_REQ_REDIS_LPOS;
                    break;
                }

                if (str4icmp(m, 'l', 'r', 'e', 'm')) {
                    r->type = MSG_REQ_REDIS_LREM;
                    break;
                }

                if (str4icmp(m, 'l', 's', 'e', 't')) {
                    r->type = MSG_REQ_REDIS_LSET;
                    break;
                }

                if (str4icmp(m, 'r', 'p', 'o', 'p')) {
                    r->type = MSG_REQ_REDIS_RPOP;
                    break;
                }

                if (str4icmp(m, 's', 'a', 'd', 'd')) {
                    r->type = MSG_REQ_REDIS_SADD;
                    break;
                }

                if (str4icmp(m, 's', 'p', 'o', 'p')) {
                    r->type = MSG_REQ_REDIS_SPOP;
                    break;
                }

                if (str4icmp(m, 's', 'r', 'e', 'm')) {
                    r->type = MSG_REQ_REDIS_SREM;
                    break;
                }

                if (str4icmp(m, 't', 'y', 'p', 'e')) {
                    r->type = MSG_REQ_REDIS_TYPE;
                    break;
                }

                if (str4icmp(m, 'm', 'g', 'e', 't')) {
                    r->type = MSG_REQ_REDIS_MGET;
                    break;
                }
                if (str4icmp(m, 'm', 's', 'e', 't')) {
                    r->type = MSG_REQ_REDIS_MSET;
                    break;
                }

                if (str4icmp(m, 'z', 'a', 'd', 'd')) {
                    r->type = MSG_REQ_REDIS_ZADD;
                    break;
                }

                if (str4icmp(m, 'z', 'r', 'e', 'm')) {
                    r->type = MSG_REQ_REDIS_ZREM;
                    break;
                }

                if (str4icmp(m, 'e', 'v', 'a', 'l')) {
                    r->type = MSG_REQ_REDIS_EVAL;
                    break;
                }

                if (str4icmp(m, 's', 'o', 'r', 't')) {
                    r->type = MSG_REQ_REDIS_SORT;
                    break;
                }

                if (str4icmp(m, 'p', 'i', 'n', 'g')) {
                    r->type = MSG_REQ_REDIS_PING;
                    r->noforward = 1;
                    break;
                }

                if (str4icmp(m, 'q', 'u', 'i', 't')) {
                    r->type = MSG_REQ_REDIS_QUIT;
                    r->quit = 1;
                    break;
                }

                if (str4icmp(m, 'a', 'u', 't', 'h')) {
                    r->type = MSG_REQ_REDIS_AUTH;
                    r->noforward = 1;
                    break;
                }

                if (str4icmp(m, 'm', 'o', 'v', 'e')) {
                    r->type = MSG_REQ_REDIS_MOVE;
                    r->noforward = 1;
                    break;
                }

                if (str4icmp(m, 'c', 'o', 'p', 'y')) {
                    r->type = MSG_REQ_REDIS_COPY;
                    break;
                }

                break;

            case 5:
                if (str5icmp(m, 'h', 'k', 'e', 'y', 's')) {
                    r->type = MSG_REQ_REDIS_HKEYS;
                    break;
                }

                if (str5icmp(m, 'h', 'm', 'g', 'e', 't')) {
                    r->type = MSG_REQ_REDIS_HMGET;
                    break;
                }

                if (str5icmp(m, 'h', 'm', 's', 'e', 't')) {
                    r->type = MSG_REQ_REDIS_HMSET;
                    break;
                }

                if (str5icmp(m, 'h', 'v', 'a', 'l', 's')) {
                    r->type = MSG_REQ_REDIS_HVALS;
                    break;
                }

                if (str5icmp(m, 'h', 's', 'c', 'a', 'n')) {
                    r->type = MSG_REQ_REDIS_HSCAN;
                    break;
                }

                if (str5icmp(m, 'l', 'p', 'u', 's', 'h')) {
                    r->type = MSG_REQ_REDIS_LPUSH;
                    break;
                }

                if (str5icmp(m, 'l', 't', 'r', 'i', 'm')) {
                    r->type = MSG_REQ_REDIS_LTRIM;
                    break;
                }

                if (str5icmp(m, 'r', 'p', 'u', 's', 'h')) {
                    r->type = MSG_REQ_REDIS_RPUSH;
                    break;
                }

                if (str5icmp(m, 's', 'c', 'a', 'r', 'd')) {
                    r->type = MSG_REQ_REDIS_SCARD;
                    break;
                }

                if (str5icmp(m, 's', 'd', 'i', 'f', 'f')) {
                    r->type = MSG_REQ_REDIS_SDIFF;
                    break;
                }

                if (str5icmp(m, 's', 'e', 't', 'e', 'x')) {
                    r->type = MSG_REQ_REDIS_SETEX;
                    break;
                }

                if (str5icmp(m, 's', 'e', 't', 'n', 'x')) {
                    r->type = MSG_REQ_REDIS_SETNX;
                    break;
                }

                if (str5icmp(m, 's', 'm', 'o', 'v', 'e')) {
                    r->type = MSG_REQ_REDIS_SMOVE;
                    break;
                }

                if (str5icmp(m, 's', 's', 'c', 'a', 'n')) {
                    r->type = MSG_REQ_REDIS_SSCAN;
                    break;
                }

                if (str5icmp(m, 'z', 'c', 'a', 'r', 'd')) {
                    r->type = MSG_REQ_REDIS_ZCARD;
                    break;
                }

                if (str5icmp(m, 'z', 'd', 'i', 'f', 'f')) {
                    r->type = MSG_REQ_REDIS_ZDIFF;
                    break;
                }

                if (str5icmp(m, 'z', 'r', 'a', 'n', 'k')) {
                    r->type = MSG_REQ_REDIS_ZRANK;
                    break;
                }

                if (str5icmp(m, 'z', 's', 'c', 'a', 'n')) {
                    r->type = MSG_REQ_REDIS_ZSCAN;
                    break;
                }

                if (str5icmp(m, 'p', 'f', 'a', 'd', 'd')) {
                    r->type = MSG_REQ_REDIS_PFADD;
                    break;
                }

                if (str5icmp(m, 'g', 'e', 't', 'e', 'x')) {
                    r->type = MSG_REQ_REDIS_GETEX;
                    break;
                }

                if (str5icmp(m, 't', 'o', 'u', 'c', 'h')) {
                    r->type = MSG_REQ_REDIS_TOUCH;
                    break;
                }

                if (str5icmp(m, 'l', 'm', 'o', 'v', 'e')) {
                    r->type = MSG_REQ_REDIS_LMOVE;
                    break;
                }

                break;

            case 6:
                if (str6icmp(m, 'a', 'p', 'p', 'e', 'n', 'd')) {
                    r->type = MSG_REQ_REDIS_APPEND;
                    break;
                }

                if (str6icmp(m, 'b', 'i', 't', 'p', 'o', 's')) {
                    r->type = MSG_REQ_REDIS_BITPOS;
                    break;
                }

                if (str6icmp(m, 'd', 'e', 'c', 'r', 'b', 'y')) {
                    r->type = MSG_REQ_REDIS_DECRBY;
                    break;
                }

                if (str6icmp(m, 'e', 'x', 'i', 's', 't', 's')) {
                    r->type = MSG_REQ_REDIS_EXISTS;
                    break;
                }

                if (str6icmp(m, 'e', 'x', 'p', 'i', 'r', 'e')) {
                    r->type = MSG_REQ_REDIS_EXPIRE;
                    break;
                }

                if (str6icmp(m, 'g', 'e', 't', 'b', 'i', 't')) {
                    r->type = MSG_REQ_REDIS_GETBIT;
                    break;
                }

                if (str6icmp(m, 'g', 'e', 't', 's', 'e', 't')) {
                    r->type = MSG_REQ_REDIS_GETSET;
                    break;
                }

                if (str6icmp(m, 'p', 's', 'e', 't', 'e', 'x')) {
                    r->type = MSG_REQ_REDIS_PSETEX;
                    break;
                }

                if (str6icmp(m, 'h', 's', 'e', 't', 'n', 'x')) {
                    r->type = MSG_REQ_REDIS_HSETNX;
                    break;
                }

                if (str6icmp(m, 'i', 'n', 'c', 'r', 'b', 'y')) {
                    r->type = MSG_REQ_REDIS_INCRBY;
                    break;
                }

                if (str6icmp(m, 'l', 'i', 'n', 'd', 'e', 'x')) {
                    r->type = MSG_REQ_REDIS_LINDEX;
                    break;
                }

                if (str6icmp(m, 'l', 'p', 'u', 's', 'h', 'x')) {
                    r->type = MSG_REQ_REDIS_LPUSHX;
                    break;
                }

                if (str6icmp(m, 'l', 'r', 'a', 'n', 'g', 'e')) {
                    r->type = MSG_REQ_REDIS_LRANGE;
                    break;
                }

                if (str6icmp(m, 'r', 'p', 'u', 's', 'h', 'x')) {
                    r->type = MSG_REQ_REDIS_RPUSHX;
                    break;
                }

                if (str6icmp(m, 's', 'e', 't', 'b', 'i', 't')) {
                    r->type = MSG_REQ_REDIS_SETBIT;
                    break;
                }

                if (str6icmp(m, 's', 'i', 'n', 't', 'e', 'r')) {
                    r->type = MSG_REQ_REDIS_SINTER;
                    break;
                }

                if (str6icmp(m, 's', 't', 'r', 'l', 'e', 'n')) {
                    r->type = MSG_REQ_REDIS_STRLEN;
                    break;
                }

                if (str6icmp(m, 's', 'u', 'n', 'i', 'o', 'n')) {
                    r->type = MSG_REQ_REDIS_SUNION;
                    break;
                }

                if (str6icmp(m, 'z', 'c', 'o', 'u', 'n', 't')) {
                    r->type = MSG_REQ_REDIS_ZCOUNT;
                    break;
                }

                if (str6icmp(m, 'z', 'r', 'a', 'n', 'g', 'e')) {
                    r->type = MSG_REQ_REDIS_ZRANGE;
                    break;
                }

                if (str6icmp(m, 'z', 's', 'c', 'o', 'r', 'e')) {
                    r->type = MSG_REQ_REDIS_ZSCORE;
                    break;
                }

                if (str6icmp(m, 'g', 'e', 'o', 'p', 'o', 's')) {
                    r->type = MSG_REQ_REDIS_GEOPOS;
                    break;
                }

                if (str6icmp(m, 'g', 'e', 'o', 'a', 'd', 'd')) {
                    r->type = MSG_REQ_REDIS_GEOADD;
                    break;
                }

                if (str6icmp(m, 'g', 'e', 't', 'd', 'e', 'l')) {
                    r->type = MSG_REQ_REDIS_GETDEL;
                    break;
                }

                if (str6icmp(m, 'z', 'u', 'n', 'i', 'o', 'n')) {
                    r->type = MSG_REQ_REDIS_ZUNION;
                    break;
                }

                if (str6icmp(m, 'z', 'i', 'n', 't', 'e', 'r')) {
                    r->type = MSG_REQ_REDIS_ZINTER;
                    break;
                }

                if (str6icmp(m, 'u', 'n', 'l', 'i', 'n', 'k')) {
                    r->type = MSG_REQ_REDIS_UNLINK;
                    break;
                }

                if (str6icmp(m, 'l', 'o', 'l', 'w', 'u', 't')) {
                    r->type = MSG_REQ_REDIS_LOLWUT;
                    if (!msg_set_placeholder_key(r)) {
                        goto enomem;
                    }
                    break;
                }

                break;

            case 7:
                if (str7icmp(m, 'p', 'e', 'r', 's', 'i', 's', 't')) {
                    r->type = MSG_REQ_REDIS_PERSIST;
                    break;
                }

                if (str7icmp(m, 'p', 'e', 'x', 'p', 'i', 'r', 'e')) {
                    r->type = MSG_REQ_REDIS_PEXPIRE;
                    break;
                }

                if (str7icmp(m, 'h', 'e', 'x', 'i', 's', 't', 's')) {
                    r->type = MSG_REQ_REDIS_HEXISTS;
                    break;
                }

                if (str7icmp(m, 'h', 'g', 'e', 't', 'a', 'l', 'l')) {
                    r->type = MSG_REQ_REDIS_HGETALL;
                    break;
                }

                if (str7icmp(m, 'h', 'i', 'n', 'c', 'r', 'b', 'y')) {
                    r->type = MSG_REQ_REDIS_HINCRBY;
                    break;
                }

                if (str7icmp(m, 'l', 'i', 'n', 's', 'e', 'r', 't')) {
                    r->type = MSG_REQ_REDIS_LINSERT;
                    break;
                }

                if (str7icmp(m, 'z', 'i', 'n', 'c', 'r', 'b', 'y')) {
                    r->type = MSG_REQ_REDIS_ZINCRBY;
                    break;
                }

                if (str7icmp(m, 'e', 'v', 'a', 'l', 's', 'h', 'a')) {
                    r->type = MSG_REQ_REDIS_EVALSHA;
                    break;
                }

                if (str7icmp(m, 'r', 'e', 's', 't', 'o', 'r', 'e')) {
                    r->type = MSG_REQ_REDIS_RESTORE;
                    break;
                }

                if (str7icmp(m, 'p', 'f', 'c', 'o', 'u', 'n', 't')) {
                    r->type = MSG_REQ_REDIS_PFCOUNT;
                    break;
                }

                if (str7icmp(m, 'p', 'f', 'm', 'e', 'r', 'g', 'e')) {
                    r->type = MSG_REQ_REDIS_PFMERGE;
                    break;
                }

                if (str7icmp(m, 'z', 'm', 's', 'c', 'o', 'r', 'e')) {
                    r->type = MSG_REQ_REDIS_ZMSCORE;
                    break;
                }

                if (str7icmp(m, 'z', 'p', 'o', 'p', 'm', 'i', 'n')) {
                    r->type = MSG_REQ_REDIS_ZPOPMIN;
                    break;
                }

                if (str7icmp(m, 'z', 'p', 'o', 'p', 'm', 'a', 'x')) {
                    r->type = MSG_REQ_REDIS_ZPOPMAX;
                    break;
                }

                if (str7icmp(m, 'g', 'e', 'o', 'd', 'i', 's', 't')) {
                    r->type = MSG_REQ_REDIS_GEODIST;
                    break;
                }

                if (str7icmp(m, 'g', 'e', 'o', 'h', 'a', 's', 'h')) {
                    r->type = MSG_REQ_REDIS_GEOHASH;
                    break;
                }

                if (str7icmp(m, 'h', 's', 't', 'r', 'l', 'e', 'n')) {
                    r->type = MSG_REQ_REDIS_HSTRLEN;
                    break;
                }

                if (str7icmp(m, 'c', 'o', 'm', 'm', 'a', 'n', 'd')) {
                    r->type = MSG_REQ_REDIS_COMMAND;
                    if (!msg_set_placeholder_key(r)) {
                        goto enomem;
                    }
                    break;
                }

                break;

            case 8:
                if (str8icmp(m, 'e', 'x', 'p', 'i', 'r', 'e', 'a', 't')) {
                    r->type = MSG_REQ_REDIS_EXPIREAT;
                    break;
                }

                if (str8icmp(m, 'b', 'i', 't', 'c', 'o', 'u', 'n', 't')) {
                    r->type = MSG_REQ_REDIS_BITCOUNT;
                    break;
                }

                if (str8icmp(m, 'g', 'e', 't', 'r', 'a', 'n', 'g', 'e')) {
                    r->type = MSG_REQ_REDIS_GETRANGE;
                    break;
                }

                if (str8icmp(m, 's', 'e', 't', 'r', 'a', 'n', 'g', 'e')) {
                    r->type = MSG_REQ_REDIS_SETRANGE;
                    break;
                }

                if (str8icmp(m, 's', 'm', 'e', 'm', 'b', 'e', 'r', 's')) {
                    r->type = MSG_REQ_REDIS_SMEMBERS;
                    break;
                }

                if (str8icmp(m, 'z', 'r', 'e', 'v', 'r', 'a', 'n', 'k')) {
                    r->type = MSG_REQ_REDIS_ZREVRANK;
                    break;
                }

                if (str8icmp(m, 'b', 'i', 't', 'f', 'i', 'e', 'l', 'd')) {
                    r->type = MSG_REQ_REDIS_BITFIELD;
                    break;
                }

                break;

            case 9:
                if (str9icmp(m, 'p', 'e', 'x', 'p', 'i', 'r', 'e', 'a', 't')) {
                    r->type = MSG_REQ_REDIS_PEXPIREAT;
                    break;
                }

                if (str9icmp(m, 'r', 'p', 'o', 'p', 'l', 'p', 'u', 's', 'h')) {
                    r->type = MSG_REQ_REDIS_RPOPLPUSH;
                    break;
                }

                if (str9icmp(m, 's', 'i', 's', 'm', 'e', 'm', 'b', 'e', 'r')) {
                    r->type = MSG_REQ_REDIS_SISMEMBER;
                    break;
                }

                if (str9icmp(m, 'z', 'r', 'e', 'v', 'r', 'a', 'n', 'g', 'e')) {
                    r->type = MSG_REQ_REDIS_ZREVRANGE;
                    break;
                }

                if (str9icmp(m, 'z', 'l', 'e', 'x', 'c', 'o', 'u', 'n', 't')) {
                    r->type = MSG_REQ_REDIS_ZLEXCOUNT;
                    break;
                }

                if (str9icmp(m, 'g', 'e', 'o', 's', 'e', 'a', 'r', 'c', 'h')) {
                    r->type = MSG_REQ_REDIS_GEOSEARCH;
                    break;
                }

                if (str9icmp(m, 'g', 'e', 'o', 'r', 'a', 'd', 'i', 'u', 's')) {
                    r->type = MSG_REQ_REDIS_GEORADIUS;
                    break;
                }

                break;

            case 10:
                if (str10icmp(m, 's', 'd', 'i', 'f', 'f', 's', 't', 'o', 'r', 'e')) {
                    r->type = MSG_REQ_REDIS_SDIFFSTORE;
                    break;
                }

                if (str10icmp(m, 'h', 'r', 'a', 'n', 'd', 'f', 'i', 'e', 'l', 'd')) {
                    r->type = MSG_REQ_REDIS_HRANDFIELD;
                    break;
                }

                if (str10icmp(m, 's', 'm', 'i', 's', 'm', 'e', 'm', 'b', 'e', 'r')) {
                    r->type = MSG_REQ_REDIS_SMISMEMBER;
                    break;
                }

                if (str10icmp(m, 'z', 'd', 'i', 'f', 'f', 's', 't', 'o', 'r', 'e')) {
                    r->type = MSG_REQ_REDIS_ZDIFFSTORE;
                    break;
                }


                break;

            case 11:
                if (str11icmp(m, 'i', 'n', 'c', 'r', 'b', 'y', 'f', 'l', 'o', 'a', 't')) {
                    r->type = MSG_REQ_REDIS_INCRBYFLOAT;
                    break;
                }

                if (str11icmp(m, 's', 'i', 'n', 't', 'e', 'r', 's', 't', 'o', 'r', 'e')) {
                    r->type = MSG_REQ_REDIS_SINTERSTORE;
                    break;
                }

                if (str11icmp(m, 's', 'r', 'a', 'n', 'd', 'm', 'e', 'm', 'b', 'e', 'r')) {
                    r->type = MSG_REQ_REDIS_SRANDMEMBER;
                    break;
                }

                if (str11icmp(m, 's', 'u', 'n', 'i', 'o', 'n', 's', 't', 'o', 'r', 'e')) {
                    r->type = MSG_REQ_REDIS_SUNIONSTORE;
                    break;
                }

                if (str11icmp(m, 'z', 'i', 'n', 't', 'e', 'r', 's', 't', 'o', 'r', 'e')) {
                    r->type = MSG_REQ_REDIS_ZINTERSTORE;
                    break;
                }

                if (str11icmp(m, 'z', 'u', 'n', 'i', 'o', 'n', 's', 't', 'o', 'r', 'e')) {
                    r->type = MSG_REQ_REDIS_ZUNIONSTORE;
                    break;
                }

                if (str11icmp(m, 'z', 'r', 'a', 'n', 'g', 'e', 'b', 'y', 'l', 'e', 'x')) {
                    r->type = MSG_REQ_REDIS_ZRANGEBYLEX;
                    break;
                }

                if (str11icmp(m, 'z', 'r', 'a', 'n', 'd', 'm', 'e', 'm', 'b', 'e', 'r')) {
                    r->type = MSG_REQ_REDIS_ZRANDMEMBER;
                    break;
                }

                if (str11icmp(m, 'z', 'r', 'a', 'n', 'g', 'e', 's', 't', 'o', 'r', 'e')) {
                    r->type = MSG_REQ_REDIS_ZRANGESTORE;
                    break;
                }

                break;

            case 12:
                if (str12icmp(m, 'h', 'i', 'n', 'c', 'r', 'b', 'y', 'f', 'l', 'o', 'a', 't')) {
                    r->type = MSG_REQ_REDIS_HINCRBYFLOAT;
                    break;
                }


                break;

            case 13:
                if (str13icmp(m, 'z', 'r', 'a', 'n', 'g', 'e', 'b', 'y', 's', 'c', 'o', 'r', 'e')) {
                    r->type = MSG_REQ_REDIS_ZRANGEBYSCORE;
                    break;
                }

                break;

            case 14:
                if (str14icmp(m, 'z', 'r', 'e', 'm', 'r', 'a', 'n', 'g', 'e', 'b', 'y', 'l', 'e', 'x')) {
                    r->type = MSG_REQ_REDIS_ZREMRANGEBYLEX;
                    break;
                }

                if (str14icmp(m, 'z', 'r', 'e', 'v', 'r', 'a', 'n', 'g', 'e', 'b', 'y', 'l', 'e', 'x')) {
                    r->type = MSG_REQ_REDIS_ZREVRANGEBYLEX;
                    break;
                }
                if (str14icmp(m, 'g', 'e', 'o', 's', 'e', 'a', 'r', 'c', 'h', 's', 't', 'o', 'r', 'e')) {
                    r->type = MSG_REQ_REDIS_GEOSEARCHSTORE;
                    break;
                }


                break;

            case 15:
                if (str15icmp(m, 'z', 'r', 'e', 'm', 'r', 'a', 'n', 'g', 'e', 'b', 'y', 'r', 'a', 'n', 'k')) {
                    r->type = MSG_REQ_REDIS_ZREMRANGEBYRANK;
                    break;
                }

                break;

            case 16:
                if (str16icmp(m, 'z', 'r', 'e', 'm', 'r', 'a', 'n', 'g', 'e', 'b', 'y', 's', 'c', 'o', 'r', 'e')) {
                    r->type = MSG_REQ_REDIS_ZREMRANGEBYSCORE;
                    break;
                }

                if (str16icmp(m, 'z', 'r', 'e', 'v', 'r', 'a', 'n', 'g', 'e', 'b', 'y', 's', 'c', 'o', 'r', 'e')) {
                    r->type = MSG_REQ_REDIS_ZREVRANGEBYSCORE;
                    break;
                }
                break;

            case 17:
                if (str17icmp(m, 'g', 'e', 'o', 'r', 'a', 'd', 'i', 'u', 's', 'b', 'y', 'm', 'e', 'm', 'b', 'e', 'r')) {
                    r->type = MSG_REQ_REDIS_GEORADIUSBYMEMBER;
                    break;
                }

            default:
                break;
            }

            if (r->type == MSG_UNKNOWN) {
                log_error("parsed unsupported command '%.*s'", (int)(p - m), m);
                goto error;
            }

            log_debug(LOG_VERB, "parsed command '%.*s'", (int)(p - m), m);

            state = SW_REQ_TYPE_LF;
            break;

        case SW_REQ_TYPE_LF:
            switch (ch) {
            case LF:
                if (redis_argz(r)) {
                    if (r->narg != 1) {
                        /* It's an error to provide more than one argument. */
                        goto error;
                    }
                    goto done;
                } else if (redis_nokey(r)) {
                    if (r->narg == 1) {
                        goto done;
                    }
                    state = SW_ARGN_LEN;
                } else if (r->narg == 1) {
                    goto error;
                } else if (redis_argeval(r)) {
                    state = SW_ARG1_LEN;
                } else {
                    state = SW_KEY_LEN;
                }
                break;

            default:
                goto error;
            }

            break;

        case SW_KEY_LEN:
            if (r->token == NULL) {
                if (ch != '$') {
                    goto error;
                }
                r->token = p;
                r->rlen = 0;
            } else if (isdigit(ch)) {
                r->rlen = r->rlen * 10 + (uint32_t)(ch - '0');
            } else if (ch == CR) {
                if (r->rlen >= mbuf_data_size()) {
                    log_error("parsed bad req %"PRIu64" of type %d with key "
                              "length %d that greater than or equal to maximum"
                              " redis key length of %zu", r->id, r->type,
                              r->rlen, mbuf_data_size());
                    goto error;
                }
                if (r->rnarg == 0) {
                    goto error;
                }
                r->rnarg--;
                r->token = NULL;
                state = SW_KEY_LEN_LF;
            } else {
                goto error;
            }

            break;

        case SW_KEY_LEN_LF:
            switch (ch) {
            case LF:
                state = SW_KEY;
                break;

            default:
                goto error;
            }

            break;

        case SW_KEY:
            if (r->token == NULL) {
                r->token = p;
            }

            m = r->token + r->rlen;
            if (m >= b->last) {
                m = b->last - 1;
                p = m;
                break;
            }

            if (*m != CR) {
                goto error;
            } else {        /* got a key */
                struct keypos *kpos;

                p = m;      /* move forward by rlen bytes */
                r->rlen = 0;
                m = r->token;
                r->token = NULL;

                kpos = array_push(r->keys);
                if (kpos == NULL) {
                    goto enomem;
                }
                kpos->start = m;
                kpos->end = p;

                state = SW_KEY_LF;
            }

            break;

        case SW_KEY_LF:
            switch (ch) {
            case LF:
                if (redis_arg0(r)) {
                    if (r->rnarg != 0) {
                        goto error;
                    }
                    goto done;
                } else if (redis_arg1(r)) {
                    if (r->rnarg != 1) {
                        goto error;
                    }
                    state = SW_ARG1_LEN;
                } else if (redis_arg2(r)) {
                    if (r->rnarg != 2) {
                        goto error;
                    }
                    state = SW_ARG1_LEN;
                } else if (redis_arg3(r)) {
                    if (r->rnarg != 3) {
                        goto error;
                    }
                    state = SW_ARG1_LEN;
                } else if (redis_argn(r)) {
                    if (r->rnarg == 0) {
                        goto done;
                    }
                    state = SW_ARG1_LEN;
                } else if (redis_argx(r)) {
                    if (r->rnarg == 0) {
                        goto done;
                    }
                    state = SW_KEY_LEN;
                } else if (redis_argkvx(r)) {
                    if (r->narg % 2 == 0) {
                        goto error;
                    }
                    state = SW_ARG1_LEN;
                } else if (redis_argeval(r)) {
                    if (r->rnarg == 0) {
                        goto done;
                    }
                    state = SW_ARGN_LEN;
                } else {
                    goto error;
                }

                break;

            default:
                goto error;
            }

            break;

        case SW_ARG1_LEN:
            if (r->token == NULL) {
                if (ch != '$') {
                    goto error;
                }
                r->rlen = 0;
                r->token = p;
            } else if (isdigit(ch)) {
                r->rlen = r->rlen * 10 + (uint32_t)(ch - '0');
            } else if (ch == CR) {
                if ((p - r->token) <= 1 || r->rnarg == 0) {
                    goto error;
                }
                r->rnarg--;
                r->token = NULL;
                state = SW_ARG1_LEN_LF;
            } else {
                goto error;
            }

            break;

        case SW_ARG1_LEN_LF:
            switch (ch) {
            case LF:
                state = SW_ARG1;
                break;

            default:
                goto error;
            }

            break;

        case SW_ARG1:
            m = p + r->rlen;
            if (m >= b->last) {
                r->rlen -= (uint32_t)(b->last - p);
                m = b->last - 1;
                p = m;
                break;
            }

            if (*m != CR) {
                goto error;
            }

            p = m; /* move forward by rlen bytes */
            r->rlen = 0;

            state = SW_ARG1_LF;

            break;

        case SW_ARG1_LF:
            switch (ch) {
            case LF:
                if (redis_arg1(r)) {
                    if (r->rnarg != 0) {
                        goto error;
                    }
                    goto done;
                } else if (redis_arg2(r)) {
                    if (r->rnarg != 1) {
                        goto error;
                    }
                    state = SW_ARG2_LEN;
                } else if (redis_arg3(r)) {
                    if (r->rnarg != 2) {
                        goto error;
                    }
                    state = SW_ARG2_LEN;
                } else if (redis_argn(r)) {
                    if (r->rnarg == 0) {
                        goto done;
                    }
                    state = SW_ARGN_LEN;
                } else if (redis_argeval(r)) {
                    if (r->rnarg < 2) {
                        goto error;
                    }
                    state = SW_ARG2_LEN;
                } else if (redis_argkvx(r)) {
                    if (r->rnarg == 0) {
                        goto done;
                    }
                    state = SW_KEY_LEN;
                } else {
                    goto error;
                }

                break;

            default:
                goto error;
            }

            break;

        case SW_ARG2_LEN:
            if (r->token == NULL) {
                if (ch != '$') {
                    goto error;
                }
                r->rlen = 0;
                r->token = p;
            } else if (isdigit(ch)) {
                r->rlen = r->rlen * 10 + (uint32_t)(ch - '0');
            } else if (ch == CR) {
                if ((p - r->token) <= 1 || r->rnarg == 0) {
                    goto error;
                }
                r->rnarg--;
                r->token = NULL;
                state = SW_ARG2_LEN_LF;
            } else {
                goto error;
            }

            break;

        case SW_ARG2_LEN_LF:
            switch (ch) {
            case LF:
                state = SW_ARG2;
                break;

            default:
                goto error;
            }

            break;

        case SW_ARG2:
            if (r->token == NULL && redis_argeval(r)) {
                /*
                 * For EVAL/EVALSHA, ARG2 represents the # key/arg pairs which must
                 * be tokenized and stored in contiguous memory.
                 */
                r->token = p;
            }

            m = p + r->rlen;
            if (m >= b->last) {
                /* 
                 * For EVAL/EVALHASH, the r->token has been assigned a value.  When
                 * m >= b->last happens will need to repair mbuf.  
                 * 
                 * At the end of redis_parse_req, r->token will be used to choose
                 * the start (p) for the next call to redis_parse_req and clear
                 * r->token when repairing this and adding more data.
                 * 
                 * So, only when r->token == NULL we need to calculate r->rlen again.
                 */
                if (r->token == NULL) {
                    r->rlen -= (uint32_t)(b->last - p);
                }
                m = b->last - 1;
                p = m;
                break;
            }

            if (*m != CR) {
                goto error;
            }

            p = m; /* move forward by rlen bytes */
            r->rlen = 0;

            if (redis_argeval(r)) {
                uint32_t nkey;
                uint8_t *chp;

                /*
                 * For EVAL/EVALSHA, we need to find the integer value of this
                 * argument. It tells us the number of keys in the script, and
                 * we need to error out if number of keys is 0. At this point,
                 * both p and m point to the end of the argument and r->token
                 * points to the start.
                 */
                if (p - r->token < 1) {
                    goto error;
                }

                for (nkey = 0, chp = r->token; chp < p; chp++) {
                    if (isdigit(*chp)) {
                        nkey = nkey * 10 + (uint32_t)(*chp - '0');
                    } else {
                        goto error;
                    }
                }
                if (nkey == 0) {
                    goto error;
                }

                r->token = NULL;
            }

            state = SW_ARG2_LF;

            break;

        case SW_ARG2_LF:
            switch (ch) {
            case LF:
                if (redis_arg2(r)) {
                    if (r->rnarg != 0) {
                        goto error;
                    }
                    goto done;
                } else if (redis_arg3(r)) {
                    if (r->rnarg != 1) {
                        goto error;
                    }
                    state = SW_ARG3_LEN;
                } else if (redis_argn(r)) {
                    if (r->rnarg == 0) {
                        goto done;
                    }
                    state = SW_ARGN_LEN;
                } else if (redis_argeval(r)) {
                    if (r->rnarg < 1) {
                        goto error;
                    }
                    state = SW_KEY_LEN;
                } else {
                    goto error;
                }

                break;

            default:
                goto error;
            }

            break;

        case SW_ARG3_LEN:
            if (r->token == NULL) {
                if (ch != '$') {
                    goto error;
                }
                r->rlen = 0;
                r->token = p;
            } else if (isdigit(ch)) {
                r->rlen = r->rlen * 10 + (uint32_t)(ch - '0');
            } else if (ch == CR) {
                if ((p - r->token) <= 1 || r->rnarg == 0) {
                    goto error;
                }
                r->rnarg--;
                r->token = NULL;
                state = SW_ARG3_LEN_LF;
            } else {
                goto error;
            }

            break;

        case SW_ARG3_LEN_LF:
            switch (ch) {
            case LF:
                state = SW_ARG3;
                break;

            default:
                goto error;
            }

            break;

        case SW_ARG3:
            m = p + r->rlen;
            if (m >= b->last) {
                r->rlen -= (uint32_t)(b->last - p);
                m = b->last - 1;
                p = m;
                break;
            }

            if (*m != CR) {
                goto error;
            }

            p = m; /* move forward by rlen bytes */
            r->rlen = 0;
            state = SW_ARG3_LF;

            break;

        case SW_ARG3_LF:
            switch (ch) {
            case LF:
                if (redis_arg3(r)) {
                    if (r->rnarg != 0) {
                        goto error;
                    }
                    goto done;
                } else if (redis_argn(r)) {
                    if (r->rnarg == 0) {
                        goto done;
                    }
                    state = SW_ARGN_LEN;
                } else {
                    goto error;
                }

                break;

            default:
                goto error;
            }

            break;

        case SW_ARGN_LEN:
            if (r->token == NULL) {
                if (ch != '$') {
                    goto error;
                }
                r->rlen = 0;
                r->token = p;
            } else if (isdigit(ch)) {
                r->rlen = r->rlen * 10 + (uint32_t)(ch - '0');
            } else if (ch == CR) {
                if ((p - r->token) <= 1 || r->rnarg == 0) {
                    goto error;
                }
                r->rnarg--;
                r->token = NULL;
                state = SW_ARGN_LEN_LF;
            } else {
                goto error;
            }

            break;

        case SW_ARGN_LEN_LF:
            switch (ch) {
            case LF:
                state = SW_ARGN;
                break;

            default:
                goto error;
            }

            break;

        case SW_ARGN:
            m = p + r->rlen;
            if (m >= b->last) {
                r->rlen -= (uint32_t)(b->last - p);
                m = b->last - 1;
                p = m;
                break;
            }

            if (*m != CR) {
                goto error;
            }

            p = m; /* move forward by rlen bytes */
            r->rlen = 0;
            state = SW_ARGN_LF;

            break;

        case SW_ARGN_LF:
            switch (ch) {
            case LF:
                if (redis_argn(r) || redis_argeval(r) || redis_nokey(r)) {
                    if (r->rnarg == 0) {
                        goto done;
                    }
                    state = SW_ARGN_LEN;
                } else {
                    goto error;
                }

                break;

            default:
                goto error;
            }

            break;

        case SW_SENTINEL:
        default:
            NOT_REACHED();
            break;
        }
    }

    ASSERT(p == b->last);
    r->pos = p;
    r->state = state;

    if (b->last == b->end && r->token != NULL) {
        r->pos = r->token;
        r->token = NULL;
        r->result = MSG_PARSE_REPAIR;
    } else {
        r->result = MSG_PARSE_AGAIN;
    }

    log_hexdump(LOG_VERB, b->pos, mbuf_length(b), "parsed req %"PRIu64" res %d "
                "type %d state %d rpos %d of %d", r->id, r->result, r->type,
                r->state, (int)(r->pos - b->pos), (int)(b->last - b->pos));
    return;

done:
    ASSERT(r->type > MSG_UNKNOWN && r->type < MSG_SENTINEL);
    r->pos = p + 1;
    ASSERT(r->pos <= b->last);
    r->state = SW_START;
    r->token = NULL;
    r->result = MSG_PARSE_OK;

    log_hexdump(LOG_VERB, b->pos, mbuf_length(b), "parsed req %"PRIu64" res %d "
                "type %d state %d rpos %d of %d", r->id, r->result, r->type,
                r->state, (int)(r->pos - b->pos), (int)(b->last - b->pos));
    return;

enomem:
    r->result = MSG_PARSE_ERROR;
    r->state = state;

    log_hexdump(LOG_INFO, b->pos, mbuf_length(b), "out of memory on parse req %"PRIu64" "
                "res %d type %d state %d", r->id, r->result, r->type, r->state);

    return;

error:
    r->result = MSG_PARSE_ERROR;
    r->state = state;
    errno = EINVAL;

    log_hexdump(LOG_INFO, b->pos, mbuf_length(b), "parsed bad req %"PRIu64" "
                "res %d type %d state %d", r->id, r->result, r->type,
                r->state);
}

/*
 * Reference: http://redis.io/topics/protocol
 *
 * Redis will reply to commands with different kinds of replies. It is
 * possible to check the kind of reply from the first byte sent by the
 * server:
 *  - with a single line reply the first byte of the reply will be "+"
 *  - with an error message the first byte of the reply will be "-"
 *  - with an integer number the first byte of the reply will be ":"
 *  - with bulk reply the first byte of the reply will be "$"
 *  - with multi-bulk reply the first byte of the reply will be "*"
 *
 * 1). Status reply (or single line reply) is in the form of a single line
 *     string starting with "+" terminated by "\r\n".
 * 2). Error reply are similar to status replies. The only difference is
 *     that the first byte is "-" instead of "+".
 * 3). Integer reply is just a CRLF terminated string representing an
 *     integer, and prefixed by a ":" byte.
 * 4). Bulk reply is used by server to return a single binary safe string.
 *     The first reply line is a "$" byte followed by the number of bytes
 *     of the actual reply, followed by CRLF, then the actual data bytes,
 *     followed by additional two bytes for the final CRLF. If the requested
 *     value does not exist the bulk reply will use the special value '-1'
 *     as the data length.
 * 5). Multi-bulk reply is used by the server to return many binary safe
 *     strings (bulks) with the initial line indicating how many bulks that
 *     will follow. The first byte of a multi bulk reply is always *.
 */
void
redis_parse_rsp(struct msg *r)
{
    struct mbuf *b;
    uint8_t *p, *m;
    uint8_t ch;

    enum {
        SW_START,
        SW_STATUS,
        SW_ERROR,
        SW_INTEGER_START,
        SW_SIMPLE,
        SW_BULK,
        SW_BULK_LF,
        SW_BULK_ARG,
        SW_BULK_ARG_LF,
        SW_MULTIBULK,
        SW_MULTIBULK_NARG_LF,
        SW_MULTIBULK_ARGN_LEN,
        SW_MULTIBULK_ARGN_LEN_LF,
        SW_MULTIBULK_ARGN,
        SW_MULTIBULK_ARGN_LF,
        SW_RUNTO_CRLF,
        SW_ALMOST_DONE,
        SW_SENTINEL
    } state;

    state = r->state;
    b = STAILQ_LAST(&r->mhdr, mbuf, next);

    ASSERT(!r->request);
    ASSERT(state >= SW_START && state < SW_SENTINEL);
    ASSERT(b != NULL);
    ASSERT(b->pos <= b->last);

    /* validate the parsing marker */
    ASSERT(r->pos != NULL);
    ASSERT(r->pos >= b->pos && r->pos <= b->last);

    for (p = r->pos; p < b->last; p++) {
        ch = *p;

        switch (state) {
        case SW_START:
            r->type = MSG_UNKNOWN;
            r->rnarg = 1;
            r->is_top_level = 1;

            switch (ch) {
            case '+':
                p = p - 1; /* go back by 1 byte */
                r->type = MSG_RSP_REDIS_STATUS;
                state = SW_STATUS;
                break;

            case '-':
                r->type = MSG_RSP_REDIS_ERROR;
                p = p - 1; /* go back by 1 byte */
                state = SW_ERROR;
                break;

            case ':':
                r->type = MSG_RSP_REDIS_INTEGER;
                r->integer = 0;
                state = SW_INTEGER_START;
                break;

            case '$':
                r->type = MSG_RSP_REDIS_BULK;
                p = p - 1; /* go back by 1 byte */
                state = SW_BULK;
                break;

            case '*':
                r->type = MSG_RSP_REDIS_MULTIBULK;
                p = p - 1; /* go back by 1 byte */
                state = SW_MULTIBULK;
                break;

            default:
                goto error;
            }

            break;

        case SW_STATUS:
            /* rsp_start <- p */
            state = SW_RUNTO_CRLF;
            break;

        case SW_ERROR:
            if (r->token == NULL) {
                if (ch != '-') {
                    goto error;
                }
              /* rsp_start <- p */
              r->token = p;
            }
            if (ch == ' ' || ch == CR) {
                m = r->token;
                r->token = NULL;
                switch (p - m) {

                case 4:
                    /*
                     * -ERR no such key\r\n
                     * -ERR syntax error\r\n
                     * -ERR source and destination objects are the same\r\n
                     * -ERR index out of range\r\n
                     */
                    if (str4cmp(m, '-', 'E', 'R', 'R')) {
                        r->type = MSG_RSP_REDIS_ERROR_ERR;
                        break;
                    }

                    /* -OOM command not allowed when used memory > 'maxmemory'.\r\n */
                    if (str4cmp(m, '-', 'O', 'O', 'M')) {
                        r->type = MSG_RSP_REDIS_ERROR_OOM;
                        break;
                    }

                    break;

                case 5:
                    /* -BUSY Redis is busy running a script. You can only call SCRIPT KILL or SHUTDOWN NOSAVE.\r\n" */
                    if (str5cmp(m, '-', 'B', 'U', 'S', 'Y')) {
                        r->type = MSG_RSP_REDIS_ERROR_BUSY;
                        break;
                    }

                    break;

                case 7:
                    /* -NOAUTH Authentication required.\r\n */
                    if (str7cmp(m, '-', 'N', 'O', 'A', 'U', 'T', 'H')) {
                        r->type = MSG_RSP_REDIS_ERROR_NOAUTH;
                        break;
                    }

                    break;

                case 8:
                    /* rsp: "-LOADING Redis is loading the dataset in memory\r\n" */
                    if (str8cmp(m, '-', 'L', 'O', 'A', 'D', 'I', 'N', 'G')) {
                        r->type = MSG_RSP_REDIS_ERROR_LOADING;
                        break;
                    }

                    /* -BUSYKEY Target key name already exists.\r\n */
                    if (str8cmp(m, '-', 'B', 'U', 'S', 'Y', 'K', 'E', 'Y')) {
                        r->type = MSG_RSP_REDIS_ERROR_BUSYKEY;
                        break;
                    }

                    /* "-MISCONF Redis is configured to save RDB snapshots, but is currently not able to persist on disk. Commands that may modify the data set are disabled. Please check Redis logs for details about the error.\r\n" */
                    if (str8cmp(m, '-', 'M', 'I', 'S', 'C', 'O', 'N', 'F')) {
                        r->type = MSG_RSP_REDIS_ERROR_MISCONF;
                        break;
                    }

                    break;

                case 9:
                    /* -NOSCRIPT No matching script. Please use EVAL.\r\n */
                    if (str9cmp(m, '-', 'N', 'O', 'S', 'C', 'R', 'I', 'P', 'T')) {
                        r->type = MSG_RSP_REDIS_ERROR_NOSCRIPT;
                        break;
                    }

                    /* -READONLY You can't write against a read only slave.\r\n */
                    if (str9cmp(m, '-', 'R', 'E', 'A', 'D', 'O', 'N', 'L', 'Y')) {
                        r->type = MSG_RSP_REDIS_ERROR_READONLY;
                        break;
                    }

                    break;

                case 10:
                    /* -WRONGTYPE Operation against a key holding the wrong kind of value\r\n */
                    if (str10cmp(m, '-', 'W', 'R', 'O', 'N', 'G', 'T', 'Y', 'P', 'E')) {
                        r->type = MSG_RSP_REDIS_ERROR_WRONGTYPE;
                        break;
                    }

                    /* -EXECABORT Transaction discarded because of previous errors.\r\n" */
                    if (str10cmp(m, '-', 'E', 'X', 'E', 'C', 'A', 'B', 'O', 'R', 'T')) {
                        r->type = MSG_RSP_REDIS_ERROR_EXECABORT;
                        break;
                    }

                    break;

                case 11:
                    /* -MASTERDOWN Link with MASTER is down and slave-serve-stale-data is set to 'no'.\r\n */
                    if (str11cmp(m, '-', 'M', 'A', 'S', 'T', 'E', 'R', 'D', 'O', 'W', 'N')) {
                        r->type = MSG_RSP_REDIS_ERROR_MASTERDOWN;
                        break;
                    }

                    /* -NOREPLICAS Not enough good slaves to write.\r\n */
                    if (str11cmp(m, '-', 'N', 'O', 'R', 'E', 'P', 'L', 'I', 'C', 'A', 'S')) {
                        r->type = MSG_RSP_REDIS_ERROR_NOREPLICAS;
                        break;
                    }

                    break;
                }
                if (ch == '\r') {
                    state = SW_ALMOST_DONE;
                } else {
                    /* Read remaining characters until '\r' */
                    state = SW_RUNTO_CRLF;
                }
            }

            break;

        case SW_SIMPLE:
            if (ch == CR) {
                ASSERT(r->rnarg > 0);
                r->rnarg--;
                state = SW_MULTIBULK_ARGN_LF;
            }
            break;

        case SW_INTEGER_START:
            if (ch == CR) {
                state = SW_ALMOST_DONE;
            } else if (ch == '-') {
                ;
            } else if (isdigit(ch)) {
                r->integer = r->integer * 10 + (uint32_t)(ch - '0');
            } else {
                goto error;
            }
            break;

        case SW_RUNTO_CRLF:
            switch (ch) {
            case CR:
                state = SW_ALMOST_DONE;
                break;

            default:
                break;
            }

            break;

        case SW_ALMOST_DONE:
            switch (ch) {
            case LF:
                /* rsp_end <- p */
                goto done;

            default:
                goto error;
            }

            break;

        case SW_BULK:
            /*
             * SW_BULK is used for top-level bulk string replies.
             * Within an array, SW_MULTIBULK_ARG... helpers are used
             * to parse bulk strings instead.
             */
            if (r->token == NULL) {
                if (ch != '$') {
                    goto error;
                }
                /* rsp_start <- p */
                r->token = p;
                r->rlen = 0;
            } else if (ch == '-') {
                /* handles null bulk reply = '$-1' */
                state = SW_RUNTO_CRLF;
            } else if (isdigit(ch)) {
                r->rlen = r->rlen * 10 + (uint32_t)(ch - '0');
            } else if (ch == CR) {
                if ((p - r->token) <= 1) {
                    goto error;
                }
                r->token = NULL;
                state = SW_BULK_LF;
            } else {
                goto error;
            }

            break;

        case SW_BULK_LF:
            switch (ch) {
            case LF:
                state = SW_BULK_ARG;
                break;

            default:
                goto error;
            }

            break;

        case SW_BULK_ARG:
            m = p + r->rlen;
            if (m >= b->last) {
                r->rlen -= (uint32_t)(b->last - p);
                m = b->last - 1;
                p = m;
                break;
            }

            if (*m != CR) {
                goto error;
            }

            p = m; /* move forward by rlen bytes */
            r->rlen = 0;

            state = SW_BULK_ARG_LF;

            break;

        case SW_BULK_ARG_LF:
            switch (ch) {
            case LF:
                goto done;

            default:
                goto error;
            }

            break;

        case SW_MULTIBULK:
            if (r->token == NULL) {
                if (ch != '*') {
                    goto error;
                }
                r->vlen = 0;
                r->token = p;
                /* rsp_start <- p */
                if (r->is_top_level) {
                    r->narg_start = p;
                }
            } else if (ch == '-') {
                p = p-1;
                r->token = NULL;
                /*
                 * This is a null array (e.g. from BLPOP). Don't increment rnarg
                 * https://redis.io/topics/protocol
                 */
                r->vlen = 1;
                state = SW_MULTIBULK_ARGN_LEN;
            } else if (isdigit(ch)) {
                r->vlen = r->vlen * 10 + (uint32_t)(ch - '0');
            } else if (ch == CR) {
                if ((p - r->token) <= 1) {
                    goto error;
                }

                if (r->is_top_level) {
                    /* For multiget responses, we may need to know the number of responses to combine them. */
                    r->narg = r->vlen;
                    r->narg_end = p;
                }
                r->is_top_level = 0;
                ASSERT(r->rnarg > 0);
                r->rnarg += r->vlen - 1;
                r->token = NULL;

                /*
                 * The stack is always initialized before transitioning
                 * to another state.
                 */
                state = SW_MULTIBULK_NARG_LF;
            } else {
                goto error;
            }

            break;

        case SW_MULTIBULK_NARG_LF:
            switch (ch) {
            case LF:
                if (r->rnarg == 0) {
                    /* response is '*0\r\n' */
                    goto done;
                }

                state = SW_MULTIBULK_ARGN_LEN;
                break;

            default:
                goto error;
            }

            break;

        case SW_MULTIBULK_ARGN_LEN:
            if (r->token == NULL) {
                /*
                 * From: http://redis.io/topics/protocol, a multi bulk reply
                 * is used to return an array of other replies. Every element
                 * of a multi bulk reply can be of any kind, including a
                 * nested multi bulk reply.
                 *
                 * Here, we only handle a multi bulk reply element that
                 * are either integer reply or bulk reply.
                 *
                 * there is a special case for sscan/hscan/zscan, these command
                 * replay a nested multi-bulk with a number and a multi bulk like this:
                 *
                 * - multi-bulk
                 *    - cursor
                 *    - multi-bulk
                 *       - val1
                 *       - val2
                 *       - val3
                 *
                 * in this case, there is only one sub-multi-bulk,
                 * and it's the last element of parent,
                 * we can handle it like tail-recursive.
                 *
                 */
                if (ch == '*') {    /* for sscan/hscan/zscan only */
                    p = p - 1;      /* go back by 1 byte */
                    state = SW_MULTIBULK;
                    break;
                }

                if (ch == ':' || ch == '+' || ch == '-') {
                    /* handles not-found reply = '$-1' or integer reply = ':<num>' */
                    /* and *2\r\n$2\r\nr0\r\n+OK\r\n or *1\r\n+OK\r\n */
                    state = SW_SIMPLE;
                    break;
                }

                if (ch != '$') {
                    goto error;
                }

                r->token = p;
                r->rlen = 0;
            } else if (isdigit(ch)) {
                r->rlen = r->rlen * 10 + (uint32_t)(ch - '0');
            } else if (ch == '-') {
                ;
            } else if (ch == CR) {
                if ((p - r->token) <= 1 || r->rnarg == 0) {
                    goto error;
                }

                if ((r->rlen == 1 && (p - r->token) == 3)) {
                    r->rlen = 0;
                    state = SW_MULTIBULK_ARGN_LF;
                } else {
                    state = SW_MULTIBULK_ARGN_LEN_LF;
                }
                ASSERT(r->rnarg > 0);
                r->rnarg--;
                r->token = NULL;
            } else {
                goto error;
            }

            break;

        case SW_MULTIBULK_ARGN_LEN_LF:
            switch (ch) {
            case LF:
                state = SW_MULTIBULK_ARGN;
                break;

            default:
                goto error;
            }

            break;

        case SW_MULTIBULK_ARGN:
            m = p + r->rlen;
            if (m >= b->last) {
                r->rlen -= (uint32_t)(b->last - p);
                m = b->last - 1;
                p = m;
                break;
            }

            if (*m != CR) {
                goto error;
            }

            p += r->rlen; /* move forward by rlen bytes */
            r->rlen = 0;

            state = SW_MULTIBULK_ARGN_LF;

            break;

        case SW_MULTIBULK_ARGN_LF:
            switch (ch) {
            case LF:
                if (r->rnarg == 0) {
                    goto done;
                }

                state = SW_MULTIBULK_ARGN_LEN;
                break;

            default:
                goto error;
            }

            break;

        case SW_SENTINEL:
        default:
            NOT_REACHED();
            break;
        }
    }

    ASSERT(p == b->last);
    r->pos = p;
    r->state = state;

    if (b->last == b->end && r->token != NULL) {
        r->pos = r->token;
        r->token = NULL;
        r->result = MSG_PARSE_REPAIR;
    } else {
        r->result = MSG_PARSE_AGAIN;
    }

    log_hexdump(LOG_VERB, b->pos, mbuf_length(b), "parsed rsp %"PRIu64" res %d "
                "type %d state %d rpos %d of %d", r->id, r->result, r->type,
                r->state, (int)(r->pos - b->pos), (int)(b->last - b->pos));
    return;

done:
    ASSERT(r->type > MSG_UNKNOWN && r->type < MSG_SENTINEL);
    r->pos = p + 1;
    ASSERT(r->pos <= b->last);
    r->state = SW_START;
    r->token = NULL;
    r->result = MSG_PARSE_OK;

    log_hexdump(LOG_VERB, b->pos, mbuf_length(b), "parsed rsp %"PRIu64" res %d "
                "type %d state %d rpos %d of %d", r->id, r->result, r->type,
                r->state, (int)(r->pos - b->pos), (int)(b->last - b->pos));
    return;

error:
    r->result = MSG_PARSE_ERROR;
    r->state = state;
    errno = EINVAL;

    log_hexdump(LOG_INFO, b->pos, mbuf_length(b), "parsed bad rsp %"PRIu64" "
                "res %d type %d state %d", r->id, r->result, r->type,
                r->state);
}

/*
 * Return true, if redis replies with a transient server failure response,
 * otherwise return false
 *
 * Transient failures on redis are scenarios when it is temporarily
 * unresponsive and responds with the following protocol specific error
 * reply:
 * -OOM, when redis is out-of-memory
 * -BUSY, when redis is busy
 * -LOADING when redis is loading dataset into memory
 *
 * See issue: https://github.com/twitter/twemproxy/issues/369
 */
bool
redis_failure(const struct msg *r)
{
    ASSERT(!r->request);

    switch (r->type) {
    case MSG_RSP_REDIS_ERROR_OOM:
    case MSG_RSP_REDIS_ERROR_BUSY:
    case MSG_RSP_REDIS_ERROR_LOADING:
        return true;

    default:
        break;
    }

    return false;
}

/*
 * copy one bulk from src to dst
 *
 * if dst == NULL, we just eat the bulk
 *
 * */
static rstatus_t
redis_copy_bulk(struct msg *dst, struct msg *src)
{
    struct mbuf *mbuf, *nbuf;
    uint8_t *p;
    uint32_t len = 0;
    uint32_t bytes = 0;
    rstatus_t status;

    for (mbuf = STAILQ_FIRST(&src->mhdr);
         mbuf && mbuf_empty(mbuf);
         mbuf = STAILQ_FIRST(&src->mhdr)) {

        mbuf_remove(&src->mhdr, mbuf);
        mbuf_put(mbuf);
    }

    mbuf = STAILQ_FIRST(&src->mhdr);
    if (mbuf == NULL) {
        return NC_ERROR;
    }

    p = mbuf->pos;
    ASSERT(*p == '$');
    p++;

    if (p[0] == '-' && p[1] == '1') {
        len = 1 + 2 + CRLF_LEN;             /* $-1\r\n */
        p = mbuf->pos + len;
    } else {
        len = 0;
        for (; p < mbuf->last && isdigit(*p); p++) {
            len = len * 10 + (uint32_t)(*p - '0');
        }
        len += CRLF_LEN * 2;
        len += (p - mbuf->pos);
    }
    bytes = len;

    /* copy len bytes to dst */
    for (; mbuf;) {
        if (mbuf_length(mbuf) <= len) {     /* steal this buf from src to dst */
            nbuf = STAILQ_NEXT(mbuf, next);
            mbuf_remove(&src->mhdr, mbuf);
            if (dst != NULL) {
                mbuf_insert(&dst->mhdr, mbuf);
            } else {
                mbuf_put(mbuf);
            }
            len -= mbuf_length(mbuf);
            mbuf = nbuf;
        } else {                             /* split it */
            if (dst != NULL) {
                status = msg_append(dst, mbuf->pos, len);
                if (status != NC_OK) {
                    return status;
                }
            }
            mbuf->pos += len;
            break;
        }
    }

    if (dst != NULL) {
        dst->mlen += bytes;
    }
    src->mlen -= bytes;
    log_debug(LOG_VVERB, "redis_copy_bulk copy bytes: %d", bytes);
    return NC_OK;
}

/*
 * Pre-coalesce handler is invoked when the message is a response to
 * the fragmented multi vector request - 'mget' or 'del' and all the
 * responses to the fragmented request vector hasn't been received
 */
void
redis_pre_coalesce(struct msg *r)
{
    struct msg *pr = r->peer; /* peer request */
    struct mbuf *mbuf;

    ASSERT(!r->request);
    ASSERT(pr->request);

    if (pr->frag_id == 0) {
        /* do nothing, if not a response to a fragmented request */
        return;
    }
    pr->frag_owner->nfrag_done++;

    switch (r->type) {
    case MSG_RSP_REDIS_INTEGER:
        /* only redis 'del' fragmented request sends back integer reply */
        ASSERT(pr->type == MSG_REQ_REDIS_DEL || pr->type == MSG_REQ_REDIS_TOUCH || pr->type == MSG_REQ_REDIS_UNLINK);

        mbuf = STAILQ_FIRST(&r->mhdr);
        /*
         * Our response parser guarantees that the integer reply will be
         * completely encapsulated in a single mbuf and we should skip over
         * all the mbuf contents and discard it as the parser has already
         * parsed the integer reply and stored it in msg->integer
         */
        ASSERT(mbuf == STAILQ_LAST(&r->mhdr, mbuf, next));
        ASSERT(r->mlen == mbuf_length(mbuf));

        r->mlen -= mbuf_length(mbuf);
        mbuf_rewind(mbuf);

        /* accumulate the integer value in frag_owner of peer request */
        pr->frag_owner->integer += r->integer;
        break;

    case MSG_RSP_REDIS_MULTIBULK:
        /* only redis 'mget' fragmented request sends back multi-bulk reply */
        ASSERT(pr->type == MSG_REQ_REDIS_MGET);

        mbuf = STAILQ_FIRST(&r->mhdr);
        /*
         * Muti-bulk reply can span over multiple mbufs and in each reply
         * we should skip over the narg token. Our response parser
         * guarantees that the narg token and the immediately following
         * '\r\n' will exist in a contiguous region in the first mbuf
         */
        ASSERT(r->narg_start == mbuf->pos);
        ASSERT(r->narg_start < r->narg_end);

        r->narg_end += CRLF_LEN;
        r->mlen -= (uint32_t)(r->narg_end - r->narg_start);
        mbuf->pos = r->narg_end;

        break;

    case MSG_RSP_REDIS_STATUS:
        if (pr->type == MSG_REQ_REDIS_MSET) {       /* MSET segments */
            mbuf = STAILQ_FIRST(&r->mhdr);
            r->mlen -= mbuf_length(mbuf);
            mbuf_rewind(mbuf);
        }
        break;

    default:
        /*
         * Valid responses for a fragmented request are MSG_RSP_REDIS_INTEGER or,
         * MSG_RSP_REDIS_MULTIBULK. For an invalid response, we send out -ERR
         * with EINVAL errno
         */
        mbuf = STAILQ_FIRST(&r->mhdr);
        log_hexdump(LOG_ERR, mbuf->pos, mbuf_length(mbuf), "rsp fragment "
                    "with unknown type %d", r->type);
        pr->error = 1;
        pr->err = EINVAL;
        break;
    }
}

static rstatus_t
redis_append_key(struct msg *r, const uint8_t *key, uint32_t keylen)
{
    uint32_t len;
    struct mbuf *mbuf;
    uint8_t printbuf[32];
    struct keypos *kpos;

    /* 1. keylen */
    len = (uint32_t)nc_snprintf(printbuf, sizeof(printbuf), "$%d\r\n", keylen);
    mbuf = msg_ensure_mbuf(r, len);
    if (mbuf == NULL) {
        return NC_ENOMEM;
    }
    mbuf_copy(mbuf, printbuf, len);
    r->mlen += len;

    /* 2. key */
    mbuf = msg_ensure_mbuf(r, keylen);
    if (mbuf == NULL) {
        return NC_ENOMEM;
    }

    kpos = array_push(r->keys);
    if (kpos == NULL) {
        return NC_ENOMEM;
    }

    kpos->start = mbuf->last;
    kpos->end = mbuf->last + keylen;
    mbuf_copy(mbuf, key, keylen);
    r->mlen += keylen;

    /* 3. CRLF */
    mbuf = msg_ensure_mbuf(r, CRLF_LEN);
    if (mbuf == NULL) {
        return NC_ENOMEM;
    }
    mbuf_copy(mbuf, (uint8_t *)CRLF, CRLF_LEN);
    r->mlen += (uint32_t)CRLF_LEN;

    return NC_OK;
}

/*
 * input a msg, return a msg chain.
 * nserver is the number of backend redis/memcache server
 *
 * the original msg will be fragmented into at most nserver fragments.
 * all the keys map to the same backend will group into one fragment.
 *
 * frag_id:
 * a unique fragment id for all fragments of the message vector. including the orig msg.
 *
 * frag_owner:
 * All fragments of the message use frag_owner point to the orig msg
 *
 * frag_seq:
 * the map from each key to it's fragment, (only in the orig msg)
 *
 * For example, a message vector with 3 keys:
 *
 *     get key1 key2 key3
 *
 * suppose we have 2 backend server, and the map is:
 *
 *     key1  => backend 0
 *     key2  => backend 1
 *     key3  => backend 0
 *
 * it will fragment like this:
 *
 *   +-----------------+
 *   |  msg vector     |
 *   |(original msg)   |
 *   |key1, key2, key3 |
 *   +-----------------+
 *
 *                                             frag_owner
 *                        /--------------------------------------+
 *       frag_owner      /                                       |
 *     /-----------+    | /------------+ frag_owner              |
 *     |           |    | |            |                         |
 *     |           v    v v            |                         |
 *   +--------------------+     +---------------------+     +----+----------------+
 *   |   frag_id = 10     |     |   frag_id = 10      |     |   frag_id = 10      |
 *   |     nfrag = 3      |     |      nfrag = 0      |     |      nfrag = 0      |
 *   | frag_seq = x x x   |     |     key1, key3      |     |         key2        |
 *   +------------|-|-|---+     +---------------------+     +---------------------+
 *                | | |          ^    ^                          ^
 *                | \ \          |    |                          |
 *                |  \ ----------+    |                          |
 *                +---\---------------+                          |
 *                     ------------------------------------------+
 *
 */
static rstatus_t
redis_fragment_argx(struct msg *r, uint32_t nserver, struct msg_tqh *frag_msgq,
                    uint32_t key_step)
{
    struct mbuf *mbuf;
    struct msg **sub_msgs;
    uint32_t i;
    rstatus_t status;
    struct array *keys = r->keys;

    ASSERT(array_n(keys) == (r->narg - 1) / key_step);

    sub_msgs = nc_zalloc(nserver * sizeof(*sub_msgs));
    if (sub_msgs == NULL) {
        return NC_ENOMEM;
    }

    ASSERT(r->frag_seq == NULL);
    r->frag_seq = nc_alloc(array_n(keys) * sizeof(*r->frag_seq));
    if (r->frag_seq == NULL) {
        nc_free(sub_msgs);
        return NC_ENOMEM;
    }

    mbuf = STAILQ_FIRST(&r->mhdr);
    mbuf->pos = mbuf->start;

    /*
     * This code is based on the assumption that '*narg\r\n$4\r\nMGET\r\n' is located
     * in a contiguous location.
     * This is always true because we have capped our MBUF_MIN_SIZE at 512 and
     * whenever we have multiple messages, we copy the tail message into a new mbuf
     */
    for (i = 0; i < 3; i++) {                 /* eat *narg\r\n$4\r\nMGET\r\n */
        for (; *(mbuf->pos) != '\n';) {
            mbuf->pos++;
        }
        mbuf->pos++;
    }

    r->frag_id = msg_gen_frag_id();
    r->nfrag = 0;
    r->frag_owner = r;

    /* Build up the key1 key2 ... to be sent to a given server at index idx */
    for (i = 0; i < array_n(keys); i++) {        /* for each key */
        struct msg *sub_msg;
        struct keypos *kpos = array_get(keys, i);
        uint32_t idx = msg_backend_idx(r, kpos->start, kpos->end - kpos->start);
        ASSERT(idx < nserver);

        if (sub_msgs[idx] == NULL) {
            sub_msgs[idx] = msg_get(r->owner, r->request, r->redis);
            if (sub_msgs[idx] == NULL) {
                nc_free(sub_msgs);
                return NC_ENOMEM;
            }
        }
        r->frag_seq[i] = sub_msg = sub_msgs[idx];

        sub_msg->narg++;
        status = redis_append_key(sub_msg, kpos->start, kpos->end - kpos->start);
        if (status != NC_OK) {
            nc_free(sub_msgs);
            return status;
        }

        if (key_step == 1) {                            /* mget,del */
            continue;
        } else {                                        /* mset */
            status = redis_copy_bulk(NULL, r);          /* eat key */
            if (status != NC_OK) {
                nc_free(sub_msgs);
                return status;
            }

            status = redis_copy_bulk(sub_msg, r);
            if (status != NC_OK) {
                nc_free(sub_msgs);
                return status;
            }

            sub_msg->narg++;
        }
    }

    /*
     * prepend mget header, and forward the command (command type+key(s)+suffix)
     * to the corresponding server(s)
     */
    for (i = 0; i < nserver; i++) {
        struct msg *sub_msg = sub_msgs[i];
        if (sub_msg == NULL) {
            continue;
        }

        if (r->type == MSG_REQ_REDIS_MGET) {
            status = msg_prepend_format(sub_msg, "*%d\r\n$4\r\nmget\r\n",
                                        sub_msg->narg + 1);
        } else if (r->type == MSG_REQ_REDIS_DEL) {
            status = msg_prepend_format(sub_msg, "*%d\r\n$3\r\ndel\r\n",
                                        sub_msg->narg + 1);
        } else if (r->type == MSG_REQ_REDIS_MSET) {
            status = msg_prepend_format(sub_msg, "*%d\r\n$4\r\nmset\r\n",
                                        sub_msg->narg + 1);
        } else if (r->type == MSG_REQ_REDIS_TOUCH) {
            status = msg_prepend_format(sub_msg, "*%d\r\n$5\r\ntouch\r\n",
                                        sub_msg->narg + 1);
        } else if (r->type == MSG_REQ_REDIS_UNLINK) {
            status = msg_prepend_format(sub_msg, "*%d\r\n$6\r\nunlink\r\n",
                                        sub_msg->narg + 1);
        } else {
            NOT_REACHED();
        }
        if (status != NC_OK) {
            nc_free(sub_msgs);
            return status;
        }

        sub_msg->type = r->type;
        sub_msg->frag_id = r->frag_id;
        sub_msg->frag_owner = r->frag_owner;

        TAILQ_INSERT_TAIL(frag_msgq, sub_msg, m_tqe);
        r->nfrag++;
    }

    nc_free(sub_msgs);
    return NC_OK;
}

rstatus_t
redis_fragment(struct msg *r, uint32_t nserver, struct msg_tqh *frag_msgq)
{
    if (1 == array_n(r->keys)){
        return NC_OK;
    }

    switch (r->type) {
    case MSG_REQ_REDIS_MGET:
    case MSG_REQ_REDIS_DEL:
    case MSG_REQ_REDIS_TOUCH:
    case MSG_REQ_REDIS_UNLINK:
        return redis_fragment_argx(r, nserver, frag_msgq, 1);

        /* TODO: MSETNX - instead of responding with OK, respond with 1 if all fragments respond with 1 */
    case MSG_REQ_REDIS_MSET:
        return redis_fragment_argx(r, nserver, frag_msgq, 2);

    default:
        return NC_OK;
    }
}

rstatus_t
redis_reply(struct msg *r)
{
    struct conn *c_conn;
    struct msg *response = r->peer;

    ASSERT(response != NULL && response->owner != NULL);

    c_conn = response->owner;
    if (r->type == MSG_REQ_REDIS_AUTH) {
        return redis_handle_auth_req(r, response);
    }

    if (!conn_authenticated(c_conn)) {
        return msg_append(response, rsp_auth_required.data, rsp_auth_required.len);
    }

    switch (r->type) {
    case MSG_REQ_REDIS_PING:
        return msg_append(response, rsp_pong.data, rsp_pong.len);

    default:
        NOT_REACHED();
        return NC_ERROR;
    }
}

void
redis_post_coalesce_mset(struct msg *request)
{
    rstatus_t status;
    struct msg *response = request->peer;

    status = msg_append(response, rsp_ok.data, rsp_ok.len);
    if (status != NC_OK) {
        response->error = 1;        /* mark this msg as err */
        response->err = errno;
    }
}

void
redis_post_coalesce_del_or_touch(struct msg *request)
{
    struct msg *response = request->peer;
    rstatus_t status;

    status = msg_prepend_format(response, ":%d\r\n", request->integer);
    if (status != NC_OK) {
        response->error = 1;
        response->err = errno;
    }
}

static void
redis_post_coalesce_mget(struct msg *request)
{
    struct msg *response = request->peer;
    struct msg *sub_msg;
    rstatus_t status;
    uint32_t i;

    status = msg_prepend_format(response, "*%d\r\n", request->narg - 1);
    if (status != NC_OK) {
        /*
         * the fragments is still in c_conn->omsg_q, we have to discard all of them,
         * we just close the conn here
         */
        response->owner->err = 1;
        return;
    }

    for (i = 0; i < array_n(request->keys); i++) {      /* for each key */
        sub_msg = request->frag_seq[i]->peer;           /* get it's peer response */
        if (sub_msg == NULL) {
            response->owner->err = 1;
            return;
        }
        status = redis_copy_bulk(response, sub_msg);
        if (status != NC_OK) {
            response->owner->err = 1;
            return;
        }
    }
}

/*
 * Post-coalesce handler is invoked when the message is a response to
 * the fragmented multi vector request - 'mget' or 'del' and all the
 * responses to the fragmented request vector has been received and
 * the fragmented request is consider to be done
 */
void
redis_post_coalesce(struct msg *r)
{
    struct msg *pr = r->peer; /* peer response */

    ASSERT(!pr->request);
    ASSERT(r->request && (r->frag_owner == r));
    if (r->error || r->ferror) {
        /* do nothing, if msg is in error */
        return;
    }

    switch (r->type) {
    case MSG_REQ_REDIS_MGET:
        return redis_post_coalesce_mget(r);

    case MSG_REQ_REDIS_DEL:
    case MSG_REQ_REDIS_TOUCH:
    case MSG_REQ_REDIS_UNLINK:
        return redis_post_coalesce_del_or_touch(r);

    case MSG_REQ_REDIS_MSET:
        return redis_post_coalesce_mset(r);

    default:
        NOT_REACHED();
    }
}

static rstatus_t
redis_handle_auth_req(struct msg *req, struct msg *rsp)
{
    struct conn *conn = (struct conn *)rsp->owner;
    const struct server_pool *pool;
    const struct keypos *kpos;
    const uint8_t *key;
    uint32_t keylen;
    bool valid;

    ASSERT(conn->client && !conn->proxy);

    pool = (const struct server_pool *)conn->owner;

    if (!pool->require_auth) {
        /*
         * AUTH command from the client in absence of a redis_auth:
         * directive should be treated as an error
         */
        return msg_append(rsp, rsp_no_password.data, rsp_no_password.len);
    }

    kpos = array_get(req->keys, 0);
    key = kpos->start;
    keylen = (uint32_t)(kpos->end - kpos->start);
    valid = (keylen == pool->redis_auth.len) &&
            (memcmp(pool->redis_auth.data, key, keylen) == 0);
    if (valid) {
        conn->authenticated = 1;
        return msg_append(rsp, rsp_ok.data, rsp_ok.len);
    }

    /*
     * Password in the AUTH command doesn't match the one configured in
     * redis_auth: directive
     *
     * We mark the connection has unauthenticated until the client
     * reauthenticates with the correct password
     */
    conn->authenticated = 0;
    return msg_append(rsp, rsp_invalid_password.data, rsp_invalid_password.len);
}

rstatus_t
redis_add_auth(struct context *ctx, struct conn *c_conn, struct conn *s_conn)
{
    rstatus_t status;
    struct msg *msg;
    struct server_pool *pool;

    ASSERT(!s_conn->client && !s_conn->proxy);
    ASSERT(!conn_authenticated(s_conn));

    pool = c_conn->owner;

    msg = msg_get(c_conn, true, c_conn->redis);
    if (msg == NULL) {
        c_conn->err = errno;
        return NC_ENOMEM;
    }

    status = msg_prepend_format(msg, "*2\r\n$4\r\nAUTH\r\n$%d\r\n%s\r\n",
                                pool->redis_auth.len, pool->redis_auth.data);
    if (status != NC_OK) {
        msg_put(msg);
        return status;
    }

    msg->swallow = 1;
    s_conn->enqueue_inq(ctx, s_conn, msg);
    s_conn->authenticated = 1;

    return NC_OK;
}

void
redis_post_connect(struct context *ctx, struct conn *conn, struct server *server)
{
    rstatus_t status;
    struct server_pool *pool = server->owner;
    struct msg *msg;
    int digits;

    ASSERT(!conn->client && conn->connected);
    ASSERT(conn->redis);

    /*
     * By default, every connection to redis uses the database DB 0. You
     * can select a different one on a per-connection basis by sending
     * a request 'SELECT <redis_db>', where <redis_db> is the configured
     * on a per pool basis in the configuration
     */
    if (pool->redis_db <= 0) {
        return;
    }

    /*
     * Create a fake client message and add it to the pipeline. We force this
     * message to be head of queue as it might already contain a command
     * that triggered the connect.
     */
    msg = msg_get(conn, true, conn->redis);
    if (msg == NULL) {
        return;
    }

    digits = (pool->redis_db >= 10) ? (int)log10(pool->redis_db) + 1 : 1;
    status = msg_prepend_format(msg, "*2\r\n$6\r\nSELECT\r\n$%d\r\n%d\r\n", digits, pool->redis_db);
    if (status != NC_OK) {
        msg_put(msg);
        return;
    }
    msg->type = MSG_REQ_REDIS_SELECT;
    msg->result = MSG_PARSE_OK;
    msg->swallow = 1;
    msg->owner = NULL;

    /* enqueue as head and send */
    req_server_enqueue_imsgq_head(ctx, conn, msg);
    msg_send(ctx, conn);

    log_debug(LOG_NOTICE, "sent 'SELECT %d' to %s | %s", pool->redis_db,
              pool->name.data, server->name.data);
}

void
redis_swallow_msg(struct conn *conn, struct msg *pmsg, struct msg *msg)
{
    if (pmsg != NULL && pmsg->type == MSG_REQ_REDIS_SELECT &&
        msg != NULL && redis_error(msg)) {
        struct server* conn_server;
        struct server_pool* conn_pool;
        struct mbuf* rsp_buffer;
        uint8_t message[128];
        size_t copy_len;

        /*
         * Get a substring from the message so that the initial - and the trailing
         * \r\n is removed.
         */
        conn_server = (struct server*)conn->owner;
        conn_pool = conn_server->owner;
        rsp_buffer = STAILQ_LAST(&msg->mhdr, mbuf, next);
        copy_len = MIN(mbuf_length(rsp_buffer) - 3, sizeof(message) - 1);

        nc_memcpy(message, &rsp_buffer->start[1], copy_len);
        message[copy_len] = 0;

        log_warn("SELECT %d failed on %s | %s: %s",
                 conn_pool->redis_db, conn_pool->name.data,
                 conn_server->name.data, message);
    }
}
