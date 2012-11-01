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

#include <nc_core.h>
#include <nc_memcache.h>

#ifdef NC_LITTLE_ENDIAN

#define str4cmp(m, c0, c1, c2, c3)                                          \
    (*(uint32_t *) m == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0))

#define str5cmp(m, c0, c1, c2, c3, c4)                                      \
    (str4cmp(m, c0, c1, c2, c3) && (m[4] == c4))

#define str6cmp(m, c0, c1, c2, c3, c4, c5)                                  \
    (str4cmp(m, c0, c1, c2, c3) &&                                          \
        (((uint32_t *) m)[1] & 0xffff) == ((c5 << 8) | c4))

#define str7cmp(m, c0, c1, c2, c3, c4, c5, c6)                              \
    (str6cmp(m, c0, c1, c2, c3, c4, c5) && (m[6] == c6))

#define str8cmp(m, c0, c1, c2, c3, c4, c5, c6, c7)                          \
    (str4cmp(m, c0, c1, c2, c3) &&                                          \
        (((uint32_t *) m)[1] == ((c7 << 24) | (c6 << 16) | (c5 << 8) | c4)))

#define str9cmp(m, c0, c1, c2, c3, c4, c5, c6, c7, c8)                      \
    (str8cmp(m, c0, c1, c2, c3, c4, c5, c6, c7) && m[8] == c8)

#define str10cmp(m, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9)                 \
    (str8cmp(m, c0, c1, c2, c3, c4, c5, c6, c7) &&                          \
        (((uint32_t *) m)[2] & 0xffff) == ((c9 << 8) | c8))

#define str11cmp(m, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10)            \
    (str10cmp(m, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9) && (m[10] == c10))

#define str12cmp(m, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11)       \
    (str8cmp(m, c0, c1, c2, c3, c4, c5, c6, c7) &&                          \
        (((uint32_t *) m)[2] == ((c11 << 24) | (c10 << 16) | (c9 << 8) | c8)))

#else

#define str4cmp(m, c0, c1, c2, c3)                                          \
    (m[0] == c0 && m[1] == c1 && m[2] == c2 && m[3] == c3)

#define str5cmp(m, c0, c1, c2, c3, c4)                                      \
    (str4cmp(m, c0, c1, c2, c3) && (m[4] == c4))

#define str6cmp(m, c0, c1, c2, c3, c4, c5)                                  \
    (str5cmp(m, c0, c1, c2, c3, c4) && m[5] == c5)

#define str7cmp(m, c0, c1, c2, c3, c4, c5, c6)                              \
    (str6cmp(m, c0, c1, c2, c3, c4, c5) && m[6] == c6)

#define str8cmp(m, c0, c1, c2, c3, c4, c5, c6, c7)                          \
    (str7cmp(m, c0, c1, c2, c3, c4, c5, c6) && m[7] == c7)

#define str9cmp(m, c0, c1, c2, c3, c4, c5, c6, c7, c8)                      \
    (str8cmp(m, c0, c1, c2, c3, c4, c5, c6, c7) && m[8] == c8)

#define str10cmp(m, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9)                 \
    (str9cmp(m, c0, c1, c2, c3, c4, c5, c6, c7, c8) && m[9] == c9)

#define str11cmp(m, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10)            \
    (str10cmp(m, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9) && m[10] == c10)

#define str12cmp(m, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11)       \
    (str11cmp(m, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10) && m[11] == c11)

#endif

/*
 * Return true, if the memcache command is a storage command, otherwise
 * return false
 */
static bool
memcache_storage(struct msg *r)
{
    switch (r->type) {
    case MSG_REQ_SET:
    case MSG_REQ_CAS:
    case MSG_REQ_ADD:
    case MSG_REQ_REPLACE:
    case MSG_REQ_APPEND:
    case MSG_REQ_PREPEND:
        return true;

    default:
        break;
    }

    return false;
}

/*
 * Return true, if the memcache command is a cas command, otherwise
 * return false
 */
static bool
memcache_cas(struct msg *r)
{
    if (r->type == MSG_REQ_CAS) {
        return true;
    }

    return false;
}

/*
 * Return true, if the memcache command is a retrieval command, otherwise
 * return false
 */
static bool
memcache_retrieval(struct msg *r)
{
    switch (r->type) {
    case MSG_REQ_GET:
    case MSG_REQ_GETS:
        return true;

    default:
        break;
    }

    return false;
}

/*
 * Return true, if the memcache command is a arithmetic command, otherwise
 * return false
 */
static bool
memcache_arithmetic(struct msg *r)
{
    switch (r->type) {
    case MSG_REQ_INCR:
    case MSG_REQ_DECR:
        return true;

    default:
        break;
    }

    return false;
}

/*
 * Return true, if the memcache command is a delete command, otherwise
 * return false
 */
static bool
memcache_delete(struct msg *r)
{
    if (r->type == MSG_REQ_DELETE) {
        return true;
    }

    return false;
}

void
memcache_parse_req(struct msg *r)
{
    struct mbuf *b;
    uint8_t *p, *m;
    uint8_t ch;
    enum {
        SW_START,
        SW_REQ_TYPE,
        SW_SPACES_BEFORE_KEY,
        SW_KEY,
        SW_SPACES_BEFORE_KEYS,
        SW_SPACES_BEFORE_FLAGS,
        SW_FLAGS,
        SW_SPACES_BEFORE_EXPIRY,
        SW_EXPIRY,
        SW_SPACES_BEFORE_VLEN,
        SW_VLEN,
        SW_SPACES_BEFORE_CAS,
        SW_CAS,
        SW_RUNTO_VAL,
        SW_VAL,
        SW_SPACES_BEFORE_NUM,
        SW_NUM,
        SW_RUNTO_CRLF,
        SW_CRLF,
        SW_NOREPLY,
        SW_AFTER_NOREPLY,
        SW_ALMOST_DONE,
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
            if (ch == ' ') {
                break;
            }

            if (ch < 'a' || ch > 'z') {
                goto error;
            }

            /* req_start <- p; type_start <- p */
            r->token = p;
            state = SW_REQ_TYPE;

            break;

        case SW_REQ_TYPE:
            if (ch == ' ' || ch == CR) {
                /* type_end = p - 1 */
                m = r->token;
                r->token = NULL;
                r->type = MSG_UNKNOWN;

                switch (p - m) {

                case 3:
                    if (str4cmp(m, 'g', 'e', 't', ' ')) {
                        r->type = MSG_REQ_GET;
                        break;
                    }

                    if (str4cmp(m, 's', 'e', 't', ' ')) {
                        r->type = MSG_REQ_SET;
                        break;
                    }

                    if (str4cmp(m, 'a', 'd', 'd', ' ')) {
                        r->type = MSG_REQ_ADD;
                        break;
                    }

                    if (str4cmp(m, 'c', 'a', 's', ' ')) {
                        r->type = MSG_REQ_CAS;
                        break;
                    }

                    break;

                case 4:
                    if (str4cmp(m, 'g', 'e', 't', 's')) {
                        r->type = MSG_REQ_GETS;
                        break;
                    }

                    if (str4cmp(m, 'i', 'n', 'c', 'r')) {
                        r->type = MSG_REQ_INCR;
                        break;
                    }

                    if (str4cmp(m, 'd', 'e', 'c', 'r')) {
                        r->type = MSG_REQ_DECR;
                        break;
                    }

                    if (str4cmp(m, 'q', 'u', 'i', 't')) {
                        r->type = MSG_REQ_QUIT;
                        r->quit = 1;
                        break;
                    }

                    break;

                case 6:
                    if (str6cmp(m, 'a', 'p', 'p', 'e', 'n', 'd')) {
                        r->type = MSG_REQ_APPEND;
                        break;
                    }

                    if (str6cmp(m, 'd', 'e', 'l', 'e', 't', 'e')) {
                        r->type = MSG_REQ_DELETE;
                        break;
                    }

                    break;

                case 7:
                    if (str7cmp(m, 'p', 'r', 'e', 'p', 'e', 'n', 'd')) {
                        r->type = MSG_REQ_PREPEND;
                        break;
                    }

                    if (str7cmp(m, 'r', 'e', 'p', 'l', 'a', 'c', 'e')) {
                        r->type = MSG_REQ_REPLACE;
                        break;
                    }

                    break;
                }

                switch (r->type) {
                case MSG_REQ_GET:
                case MSG_REQ_GETS:
                case MSG_REQ_DELETE:
                case MSG_REQ_CAS:
                case MSG_REQ_SET:
                case MSG_REQ_ADD:
                case MSG_REQ_REPLACE:
                case MSG_REQ_APPEND:
                case MSG_REQ_PREPEND:
                case MSG_REQ_INCR:
                case MSG_REQ_DECR:
                    if (ch == CR) {
                        goto error;
                    }
                    state = SW_SPACES_BEFORE_KEY;
                    break;

                case MSG_REQ_QUIT:
                    p = p - 1; /* go back by 1 byte */
                    state = SW_CRLF;
                    break;

                case MSG_UNKNOWN:
                    goto error;

                default:
                    NOT_REACHED();
                }

            } else if (ch < 'a' || ch > 'z') {
                goto error;
            }

            break;

        case SW_SPACES_BEFORE_KEY:
            if (ch != ' ') {
                r->token = p;
                r->key_start = p;
                state = SW_KEY;
            }

            break;

        case SW_KEY:
            if (ch == ' ' || ch == CR) {
                if ((p - r->key_start) > MEMCACHE_MAX_KEY_LENGTH) {
                    log_error("parsed bad req %"PRIu64" of type %d with key "
                              "prefix '%.*s...' and length %d that exceeds "
                              "maximum key length", r->id, r->type, 16,
                              r->key_start, p - r->key_start);
                    goto error;
                }
                r->key_end = p - 1;
                r->token = NULL;

                /* get next state */
                if (memcache_storage(r)) {
                    state = SW_SPACES_BEFORE_FLAGS;
                } else if (memcache_arithmetic(r)) {
                    state = SW_SPACES_BEFORE_NUM;
                } else if (memcache_delete(r)) {
                    state = SW_RUNTO_CRLF;
                } else if (memcache_retrieval(r)) {
                    state = SW_SPACES_BEFORE_KEYS;
                } else {
                    state = SW_RUNTO_CRLF;
                }

                if (ch == CR) {
                    if (memcache_storage(r) || memcache_arithmetic(r)) {
                        goto error;
                    }
                    p = p - 1; /* go back by 1 byte */
                }
            }

            break;

        case SW_SPACES_BEFORE_KEYS:
            ASSERT(memcache_retrieval(r));
            switch (ch) {
            case ' ':
                break;

            case CR:
                state = SW_ALMOST_DONE;
                break;

            default:
                r->token = p;
                goto fragment;
            }

            break;

        case SW_SPACES_BEFORE_FLAGS:
            if (ch != ' ') {
                if (ch < '0' || ch > '9') {
                    goto error;
                }
                /* flags_start <- p; flags <- ch - '0' */
                r->token = p;
                state = SW_FLAGS;
            }

            break;

        case SW_FLAGS:
            if (ch >= '0' && ch <= '9') {
                /* flags <- flags * 10 + (ch - '0') */
                ;
            } else if (ch == ' ') {
                /* flags_end <- p - 1 */
                r->token = NULL;
                state = SW_SPACES_BEFORE_EXPIRY;
            } else {
                goto error;
            }

            break;

        case SW_SPACES_BEFORE_EXPIRY:
            if (ch != ' ') {
                if (ch < '0' || ch > '9') {
                    goto error;
                }
                /* expiry_start <- p; expiry <- ch - '0' */
                r->token = p;
                state = SW_EXPIRY;
            }

            break;

        case SW_EXPIRY:
            if (ch >= '0' && ch <= '9') {
                /* expiry <- expiry * 10 + (ch - '0') */
                ;
            } else if (ch == ' ') {
                /* expiry_end <- p - 1 */
                r->token = NULL;
                state = SW_SPACES_BEFORE_VLEN;
            } else {
                goto error;
            }

            break;

        case SW_SPACES_BEFORE_VLEN:
            if (ch != ' ') {
                if (ch < '0' || ch > '9') {
                    goto error;
                }
                /* vlen_start <- p */
                r->token = p;
                r->vlen = (uint32_t)(ch - '0');
                state = SW_VLEN;
            }

            break;

        case SW_VLEN:
            if (ch >= '0' && ch <= '9') {
                r->vlen = r->vlen * 10 + (uint32_t)(ch - '0');
            } else if (memcache_cas(r)) {
                if (ch != ' ') {
                    goto error;
                }
                /* vlen_end <- p - 1 */
                p = p - 1; /* go back by 1 byte */
                r->token = NULL;
                state = SW_SPACES_BEFORE_CAS;
            } else if (ch == ' ' || ch == CR) {
                /* vlen_end <- p - 1 */
                p = p - 1; /* go back by 1 byte */
                r->token = NULL;
                state = SW_RUNTO_CRLF;
            } else {
                goto error;
            }

            break;

        case SW_SPACES_BEFORE_CAS:
            if (ch != ' ') {
                if (ch < '0' || ch > '9') {
                    goto error;
                }
                /* cas_start <- p; cas <- ch - '0' */
                r->token = p;
                state = SW_CAS;
            }

            break;

        case SW_CAS:
            if (ch >= '0' && ch <= '9') {
                /* cas <- cas * 10 + (ch - '0') */
                ;
            } else if (ch == ' ' || ch == CR) {
                /* cas_end <- p - 1 */
                p = p - 1; /* go back by 1 byte */
                r->token = NULL;
                state = SW_RUNTO_CRLF;
            } else {
                goto error;
            }

            break;


        case SW_RUNTO_VAL:
            switch (ch) {
            case LF:
                /* val_start <- p + 1 */
                state = SW_VAL;
                break;

            default:
                goto error;
            }

            break;

        case SW_VAL:
            m = p + r->vlen;
            if (m >= b->last) {
                ASSERT(r->vlen >= (uint32_t)(b->last - p));
                r->vlen -= (uint32_t)(b->last - p);
                m = b->last - 1;
                p = m; /* move forward by vlen bytes */
                break;
            }
            switch (*m) {
            case CR:
                /* val_end <- p - 1 */
                p = m; /* move forward by vlen bytes */
                state = SW_ALMOST_DONE;
                break;

            default:
                goto error;
            }

            break;

        case SW_SPACES_BEFORE_NUM:
            if (ch != ' ') {
                if (ch < '0' || ch > '9') {
                    goto error;
                }
                /* num_start <- p; num <- ch - '0'  */
                r->token = p;
                state = SW_NUM;
            }

            break;

        case SW_NUM:
            if (ch >= '0' && ch <= '9') {
                /* num <- num * 10 + (ch - '0') */
                ;
            } else if (ch == ' ' || ch == CR) {
                r->token = NULL;
                /* num_end <- p - 1 */
                p = p - 1; /* go back by 1 byte */
                state = SW_RUNTO_CRLF;
            } else {
                goto error;
            }

            break;

        case SW_RUNTO_CRLF:
            switch (ch) {
            case ' ':
                break;

            case 'n':
                if (memcache_storage(r) || memcache_arithmetic(r) || memcache_delete(r)) {
                    /* noreply_start <- p */
                    r->token = p;
                    state = SW_NOREPLY;
                } else {
                    goto error;
                }

                break;

            case CR:
                if (memcache_storage(r)) {
                    state = SW_RUNTO_VAL;
                } else {
                    state = SW_ALMOST_DONE;
                }

                break;

            default:
                goto error;
            }

            break;

        case SW_NOREPLY:
            switch (ch) {
            case ' ':
            case CR:
                m = r->token;
                if (((p - m) == 7) && str7cmp(m, 'n', 'o', 'r', 'e', 'p', 'l', 'y')) {
                    ASSERT(memcache_storage(r) || memcache_arithmetic(r) || memcache_delete(r));
                    r->token = NULL;
                    /* noreply_end <- p - 1 */
                    r->noreply = 1;
                    state = SW_AFTER_NOREPLY;
                    p = p - 1; /* go back by 1 byte */
                } else {
                    goto error;
                }
            }

            break;

        case SW_AFTER_NOREPLY:
            switch (ch) {
            case ' ':
                break;

            case CR:
                if (memcache_storage(r)) {
                    state = SW_RUNTO_VAL;
                } else {
                    state = SW_ALMOST_DONE;
                }
                break;

            default:
                goto error;
            }

            break;

        case SW_CRLF:
            switch (ch) {
            case ' ':
                break;

            case CR:
                state = SW_ALMOST_DONE;
                break;

            default:
                goto error;
            }

            break;

        case SW_ALMOST_DONE:
            switch (ch) {
            case LF:
                /* req_end <- p */
                goto done;

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

    /*
     * At this point, buffer from b->pos to b->last has been parsed completely
     * but we haven't been able to reach to any conclusion. Normally, this
     * means that we have to parse again starting from the state we are in
     * after more data has been read. The newly read data is either read into
     * a new mbuf, if existing mbuf is full (b->last == b->end) or into the
     * existing mbuf.
     *
     * The only exception to this is when the existing mbuf is full (b->last
     * is at b->end) and token marker is set, which means that we have to
     * copy the partial token into a new mbuf and parse again with more data
     * read into new mbuf.
     */
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
                r->state, r->pos - b->pos, b->last - b->pos);
    return;

fragment:
    ASSERT(p != b->last);
    ASSERT(r->token != NULL);
    r->pos = r->token;
    r->token = NULL;
    r->state = state;
    r->result = MSG_PARSE_FRAGMENT;

    log_hexdump(LOG_VERB, b->pos, mbuf_length(b), "parsed req %"PRIu64" res %d "
                "type %d state %d rpos %d of %d", r->id, r->result, r->type,
                r->state, r->pos - b->pos, b->last - b->pos);
    return;

done:
    ASSERT(r->type > MSG_UNKNOWN && r->type < MSG_SENTINEL);
    r->pos = p + 1;
    ASSERT(r->pos <= b->last);
    r->state = SW_START;
    r->result = MSG_PARSE_OK;

    log_hexdump(LOG_VERB, b->pos, mbuf_length(b), "parsed req %"PRIu64" res %d "
                "type %d state %d rpos %d of %d", r->id, r->result, r->type,
                r->state, r->pos - b->pos, b->last - b->pos);
    return;

error:
    r->result = MSG_PARSE_ERROR;
    r->state = state;
    errno = EINVAL;

    log_hexdump(LOG_INFO, b->pos, mbuf_length(b), "parsed bad req %"PRIu64" "
                "res %d type %d state %d", r->id, r->result, r->type,
                r->state);
}

void
memcache_parse_rsp(struct msg *r)
{
    struct mbuf *b;
    uint8_t *p, *m;
    uint8_t ch;
    enum {
        SW_START,
        SW_RSP_NUM,
        SW_RSP_STR,
        SW_SPACES_BEFORE_KEY,
        SW_KEY,
        SW_SPACES_BEFORE_FLAGS,
        SW_FLAGS,
        SW_SPACES_BEFORE_VLEN,
        SW_VLEN,
        SW_RUNTO_VAL,
        SW_VAL,
        SW_RUNTO_END,
        SW_E,
        SW_EN,
        SW_END,
        SW_RUNTO_CRLF,
        SW_CRLF,
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
            /* rsp_start <- p; type_start <- p */
            r->token = p;
            if (ch >= '0' && ch <= '9') {
                /* num <- ch - '0' */
                state = SW_RSP_NUM;
            } else {
                state = SW_RSP_STR;
            }

            break;

        case SW_RSP_NUM:
            if (ch >= '0' && ch <= '9') {
                /* num <- num * 10 + (ch - '0') */
                ;
            } else if (ch == ' ' || ch == CR) {
                /* type_end <- p - 1 */
                r->token = NULL;
                r->type = MSG_RSP_NUM;
                p = p - 1; /* go back by 1 byte */
                state = SW_CRLF;
            } else {
                goto error;
            }

            break;

        case SW_RSP_STR:
            if (ch == ' ' || ch == CR) {
                /* type_end <- p - 1 */
                m = r->token;
                r->token = NULL;
                r->type = MSG_UNKNOWN;

                switch (p - m) {
                case 3:
                    if (str4cmp(m, 'E', 'N', 'D', '\r')) {
                        r->type = MSG_RSP_END;
                        /* end_start <- m; end_end <- p - 1*/
                        r->end = m;
                        break;
                    }

                    break;

                case 5:
                    if (str5cmp(m, 'V', 'A', 'L', 'U', 'E')) {
                        /*
                         * Encompasses responses for 'get', 'gets' and
                         * 'cas' command.
                         */
                        r->type = MSG_RSP_VALUE;
                        break;
                    }

                    if (str5cmp(m, 'E', 'R', 'R', 'O', 'R')) {
                        r->type = MSG_RSP_ERROR;
                        break;
                    }

                    break;

                case 6:
                    if (str6cmp(m, 'S', 'T', 'O', 'R', 'E', 'D')) {
                        r->type = MSG_RSP_STORED;
                        break;
                    }

                    if (str6cmp(m, 'E', 'X', 'I', 'S', 'T', 'S')) {
                        r->type = MSG_RSP_EXISTS;
                        break;
                    }

                    break;

                case 7:
                    if (str7cmp(m, 'D', 'E', 'L', 'E', 'T', 'E', 'D')) {
                        r->type = MSG_RSP_DELETED;
                        break;
                    }

                    break;

                case 9:
                    if (str9cmp(m, 'N', 'O', 'T', '_', 'F', 'O', 'U', 'N', 'D')) {
                        r->type = MSG_RSP_NOT_FOUND;
                        break;
                    }

                    break;

                case 10:
                    if (str10cmp(m, 'N', 'O', 'T', '_', 'S', 'T', 'O', 'R', 'E', 'D')) {
                        r->type = MSG_RSP_NOT_STORED;
                        break;
                    }

                    break;

                case 12:
                    if (str12cmp(m, 'C', 'L', 'I', 'E', 'N', 'T', '_', 'E', 'R', 'R', 'O', 'R')) {
                        r->type = MSG_RSP_CLIENT_ERROR;
                        break;
                    }

                    if (str12cmp(m, 'S', 'E', 'R', 'V', 'E', 'R', '_', 'E', 'R', 'R', 'O', 'R')) {
                        r->type = MSG_RSP_SERVER_ERROR;
                        break;
                    }

                    break;
                }

                switch (r->type) {
                case MSG_UNKNOWN:
                    goto error;

                case MSG_RSP_STORED:
                case MSG_RSP_NOT_STORED:
                case MSG_RSP_EXISTS:
                case MSG_RSP_NOT_FOUND:
                case MSG_RSP_DELETED:
                    state = SW_CRLF;
                    break;

                case MSG_RSP_END:
                    state = SW_CRLF;
                    break;

                case MSG_RSP_VALUE:
                    state = SW_SPACES_BEFORE_KEY;
                    break;

                case MSG_RSP_ERROR:
                    state = SW_CRLF;
                    break;

                case MSG_RSP_CLIENT_ERROR:
                case MSG_RSP_SERVER_ERROR:
                    state = SW_RUNTO_CRLF;
                    break;

                default:
                    NOT_REACHED();
                }

                p = p - 1; /* go back by 1 byte */
            }

            break;

        case SW_SPACES_BEFORE_KEY:
            if (ch != ' ') {
                r->token = p;
                r->key_start = p;
                state = SW_KEY;
            }

            break;

        case SW_KEY:
            if (ch == ' ') {
                if ((p - r->key_start) > MEMCACHE_MAX_KEY_LENGTH) {
                    log_error("parsed bad req %"PRIu64" of type %d with key "
                              "prefix '%.*s...' and length %d that exceeds "
                              "maximum key length", r->id, r->type, 16,
                              r->key_start, p - r->key_start);
                    goto error;
                }
                r->key_end = p - 1;
                r->token = NULL;
                state = SW_SPACES_BEFORE_FLAGS;
            }

            break;

        case SW_SPACES_BEFORE_FLAGS:
            if (ch != ' ') {
                if (ch < '0' || ch > '9') {
                    goto error;
                }
                /* flags_start <- p; flags <- ch - '0' */
                r->token = p;
                state = SW_FLAGS;
            }

            break;

        case SW_FLAGS:
            if (ch >= '0' && ch <= '9') {
                /* flags <- flags * 10 + (ch - '0') */
                ;
            } else if (ch == ' ') {
                /* flags_end <- p - 1 */
                r->token = NULL;
                state = SW_SPACES_BEFORE_VLEN;
            } else {
                goto error;
            }

            break;

        case SW_SPACES_BEFORE_VLEN:
            if (ch != ' ') {
                if (ch < '0' || ch > '9') {
                    goto error;
                }
                /* vlen_start <- p */
                r->token = p;
                r->vlen = (uint32_t)(ch - '0');
                state = SW_VLEN;
            }

            break;

        case SW_VLEN:
            if (ch >= '0' && ch <= '9') {
                r->vlen = r->vlen * 10 + (uint32_t)(ch - '0');
            } else if (ch == ' ' || ch == CR) {
                /* vlen_end <- p - 1 */
                p = p - 1; /* go back by 1 byte */
                r->token = NULL;
                state = SW_RUNTO_CRLF;
            } else {
                goto error;
            }

            break;

        case SW_RUNTO_VAL:
            switch (ch) {
            case LF:
                /* val_start <- p + 1 */
                state = SW_VAL;
                break;

            default:
                goto error;
            }

            break;

        case SW_VAL:
            m = p + r->vlen;
            if (m >= b->last) {
                ASSERT(r->vlen >= (uint32_t)(b->last - p));
                r->vlen -= (uint32_t)(b->last - p);
                m = b->last - 1;
                p = m; /* move forward by vlen bytes */
                break;
            }
            switch (*m) {
            case CR:
                /* val_end <- p - 1 */
                p = m; /* move forward by vlen bytes */
                state = SW_RUNTO_END;
                break;

            default:
                goto error;
            }

            break;

        case SW_RUNTO_END:
            switch (ch) {
            case LF:
                state = SW_E;
                break;

            default:
                goto error;
            }

            break;

        case SW_E:
            switch (ch) {
            case 'E':
                /* end_start <- p */
                r->token = p;
                state = SW_EN;
                break;

            default:
                goto error;
            }

            break;

        case SW_EN:
            switch (ch) {
            case 'N':
                state = SW_END;
                break;

            default:
                goto error;
            }

            break;

        case SW_END:
            switch (ch) {
            case 'D':
                /* end_end <- p */
                r->token = NULL;
                r->end = p - 2; /* go back by 2 bytes to get to end marker */
                state = SW_CRLF;
                break;

            default:
                goto error;
            }

            break;

        case SW_RUNTO_CRLF:
            switch (ch) {
            case CR:
                if (r->type == MSG_RSP_VALUE) {
                    state = SW_RUNTO_VAL;
                } else {
                    state = SW_ALMOST_DONE;
                }

                break;

            default:
                break;
            }

            break;

        case SW_CRLF:
            switch (ch) {
            case ' ':
                break;

            case CR:
                state = SW_ALMOST_DONE;
                break;

            default:
                goto error;
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
                r->state, r->pos - b->pos, b->last - b->pos);
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
                r->state, r->pos - b->pos, b->last - b->pos);
    return;

error:
    r->result = MSG_PARSE_ERROR;
    r->state = state;
    errno = EINVAL;

    log_hexdump(LOG_INFO, b->pos, mbuf_length(b), "parsed bad rsp %"PRIu64" "
                "res %d type %d state %d", r->id, r->result, r->type,
                r->state);
}
