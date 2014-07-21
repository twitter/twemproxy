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

#include <ctype.h>

#include <nc_core.h>
#include <nc_proto.h>

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
#define MEMCACHE_MAX_KEY_LENGTH 250

/*
 * Return true, if the memcache command is a storage command, otherwise
 * return false
 */
static bool
memcache_storage(struct msg *r)
{
    switch (r->type) {
    case MSG_REQ_MC_SET:
    case MSG_REQ_MC_CAS:
    case MSG_REQ_MC_ADD:
    case MSG_REQ_MC_REPLACE:
    case MSG_REQ_MC_APPEND:
    case MSG_REQ_MC_PREPEND:
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
    if (r->type == MSG_REQ_MC_CAS) {
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
    case MSG_REQ_MC_GET:
    case MSG_REQ_MC_GETS:
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
    case MSG_REQ_MC_INCR:
    case MSG_REQ_MC_DECR:
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
    if (r->type == MSG_REQ_MC_DELETE) {
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
    ASSERT(!r->redis);
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

            if (!islower(ch)) {
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
                r->narg++;

                switch (p - m) {

                case 3:
                    if (str4cmp(m, 'g', 'e', 't', ' ')) {
                        r->type = MSG_REQ_MC_GET;
                        break;
                    }

                    if (str4cmp(m, 's', 'e', 't', ' ')) {
                        r->type = MSG_REQ_MC_SET;
                        break;
                    }

                    if (str4cmp(m, 'a', 'd', 'd', ' ')) {
                        r->type = MSG_REQ_MC_ADD;
                        break;
                    }

                    if (str4cmp(m, 'c', 'a', 's', ' ')) {
                        r->type = MSG_REQ_MC_CAS;
                        break;
                    }

                    break;

                case 4:
                    if (str4cmp(m, 'g', 'e', 't', 's')) {
                        r->type = MSG_REQ_MC_GETS;
                        break;
                    }

                    if (str4cmp(m, 'i', 'n', 'c', 'r')) {
                        r->type = MSG_REQ_MC_INCR;
                        break;
                    }

                    if (str4cmp(m, 'd', 'e', 'c', 'r')) {
                        r->type = MSG_REQ_MC_DECR;
                        break;
                    }

                    if (str4cmp(m, 'q', 'u', 'i', 't')) {
                        r->type = MSG_REQ_MC_QUIT;
                        r->quit = 1;
                        break;
                    }

                    break;

                case 6:
                    if (str6cmp(m, 'a', 'p', 'p', 'e', 'n', 'd')) {
                        r->type = MSG_REQ_MC_APPEND;
                        break;
                    }

                    if (str6cmp(m, 'd', 'e', 'l', 'e', 't', 'e')) {
                        r->type = MSG_REQ_MC_DELETE;
                        break;
                    }

                    break;

                case 7:
                    if (str7cmp(m, 'p', 'r', 'e', 'p', 'e', 'n', 'd')) {
                        r->type = MSG_REQ_MC_PREPEND;
                        break;
                    }

                    if (str7cmp(m, 'r', 'e', 'p', 'l', 'a', 'c', 'e')) {
                        r->type = MSG_REQ_MC_REPLACE;
                        break;
                    }

                    break;
                }

                switch (r->type) {
                case MSG_REQ_MC_GET:
                case MSG_REQ_MC_GETS:
                case MSG_REQ_MC_DELETE:
                case MSG_REQ_MC_CAS:
                case MSG_REQ_MC_SET:
                case MSG_REQ_MC_ADD:
                case MSG_REQ_MC_REPLACE:
                case MSG_REQ_MC_APPEND:
                case MSG_REQ_MC_PREPEND:
                case MSG_REQ_MC_INCR:
                case MSG_REQ_MC_DECR:
                    if (ch == CR) {
                        goto error;
                    }
                    state = SW_SPACES_BEFORE_KEY;
                    break;

                case MSG_REQ_MC_QUIT:
                    p = p - 1; /* go back by 1 byte */
                    state = SW_CRLF;
                    break;

                case MSG_UNKNOWN:
                    goto error;

                default:
                    NOT_REACHED();
                }

            } else if (!islower(ch)) {
                goto error;
            }

            break;

        case SW_SPACES_BEFORE_KEY:
            if (ch != ' ') {
                p = p - 1; /* go back by 1 byte */
                r->token = NULL;
                state = SW_KEY;
            }
            break;

        case SW_KEY:
            if (r->token == NULL) {
                r->token = p;
                r->key_start = p;
            }
            if (ch == ' ' || ch == CR) {
                if ((p - r->key_start) > MEMCACHE_MAX_KEY_LENGTH) {
                    log_error("parsed bad req %"PRIu64" of type %d with key "
                              "prefix '%.*s...' and length %d that exceeds "
                              "maximum key length", r->id, r->type, 16,
                              r->key_start, p - r->key_start);
                    goto error;
                }
                r->narg++;
                r->key_end = p;
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
                r->token = NULL;
                p = p - 1; /* go back by 1 byte */
                state = SW_KEY;
                /* goto fragment; */
            }

            break;

        case SW_SPACES_BEFORE_FLAGS:
            if (ch != ' ') {
                if (!isdigit(ch)) {
                    goto error;
                }
                /* flags_start <- p; flags <- ch - '0' */
                r->token = p;
                state = SW_FLAGS;
            }

            break;

        case SW_FLAGS:
            if (isdigit(ch)) {
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
                if (!isdigit(ch)) {
                    goto error;
                }
                /* expiry_start <- p; expiry <- ch - '0' */
                r->token = p;
                state = SW_EXPIRY;
            }

            break;

        case SW_EXPIRY:
            if (isdigit(ch)) {
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
                if (!isdigit(ch)) {
                    goto error;
                }
                /* vlen_start <- p */
                r->vlen = (uint32_t)(ch - '0');
                state = SW_VLEN;
            }

            break;

        case SW_VLEN:
            if (isdigit(ch)) {
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
                if (!isdigit(ch)) {
                    goto error;
                }
                /* cas_start <- p; cas <- ch - '0' */
                r->token = p;
                state = SW_CAS;
            }

            break;

        case SW_CAS:
            if (isdigit(ch)) {
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
                if (!isdigit(ch)) {
                    goto error;
                }
                /* num_start <- p; num <- ch - '0'  */
                r->token = p;
                state = SW_NUM;
            }

            break;

        case SW_NUM:
            if (isdigit(ch)) {
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
        SW_SPACES_BEFORE_FLAGS,     /* 5 */
        SW_FLAGS,
        SW_SPACES_BEFORE_VLEN,
        SW_VLEN,
        SW_RUNTO_VAL,
        SW_VAL,                     /* 10 */
        SW_VAL_LF,
        SW_END,
        SW_RUNTO_CRLF,
        SW_CRLF,
        SW_ALMOST_DONE,             /* 15 */
        SW_SENTINEL
    } state;

    state = r->state;
    b = STAILQ_LAST(&r->mhdr, mbuf, next);

    ASSERT(!r->request);
    ASSERT(!r->redis);
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
            if (isdigit(ch)) {
                state = SW_RSP_NUM;
            } else {
                state = SW_RSP_STR;
            }
            p = p - 1; /* go back by 1 byte */

            break;

        case SW_RSP_NUM:
            if (r->token == NULL) {
                /* rsp_start <- p; type_start <- p */
                r->token = p;
            }

            if (isdigit(ch)) {
                /* num <- num * 10 + (ch - '0') */
                ;
            } else if (ch == ' ' || ch == CR) {
                /* type_end <- p - 1 */
                r->token = NULL;
                r->type = MSG_RSP_MC_NUM;
                p = p - 1; /* go back by 1 byte */
                state = SW_CRLF;
            } else {
                goto error;
            }

            break;

        case SW_RSP_STR:
            if (r->token == NULL) {
                /* rsp_start <- p; type_start <- p */
                r->token = p;
            }

            if (ch == ' ' || ch == CR) {
                /* type_end <- p - 1 */
                m = r->token;
                /* r->token = NULL; */
                r->type = MSG_UNKNOWN;

                switch (p - m) {
                case 3:
                    if (str4cmp(m, 'E', 'N', 'D', '\r')) {
                        r->type = MSG_RSP_MC_END;
                        /* end_start <- m; end_end <- p - 1 */
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
                        r->type = MSG_RSP_MC_VALUE;
                        break;
                    }

                    if (str5cmp(m, 'E', 'R', 'R', 'O', 'R')) {
                        r->type = MSG_RSP_MC_ERROR;
                        break;
                    }

                    break;

                case 6:
                    if (str6cmp(m, 'S', 'T', 'O', 'R', 'E', 'D')) {
                        r->type = MSG_RSP_MC_STORED;
                        break;
                    }

                    if (str6cmp(m, 'E', 'X', 'I', 'S', 'T', 'S')) {
                        r->type = MSG_RSP_MC_EXISTS;
                        break;
                    }

                    break;

                case 7:
                    if (str7cmp(m, 'D', 'E', 'L', 'E', 'T', 'E', 'D')) {
                        r->type = MSG_RSP_MC_DELETED;
                        break;
                    }

                    break;

                case 9:
                    if (str9cmp(m, 'N', 'O', 'T', '_', 'F', 'O', 'U', 'N', 'D')) {
                        r->type = MSG_RSP_MC_NOT_FOUND;
                        break;
                    }

                    break;

                case 10:
                    if (str10cmp(m, 'N', 'O', 'T', '_', 'S', 'T', 'O', 'R', 'E', 'D')) {
                        r->type = MSG_RSP_MC_NOT_STORED;
                        break;
                    }

                    break;

                case 12:
                    if (str12cmp(m, 'C', 'L', 'I', 'E', 'N', 'T', '_', 'E', 'R', 'R', 'O', 'R')) {
                        r->type = MSG_RSP_MC_CLIENT_ERROR;
                        break;
                    }

                    if (str12cmp(m, 'S', 'E', 'R', 'V', 'E', 'R', '_', 'E', 'R', 'R', 'O', 'R')) {
                        r->type = MSG_RSP_MC_SERVER_ERROR;
                        break;
                    }

                    break;
                }

                switch (r->type) {
                case MSG_UNKNOWN:
                    goto error;

                case MSG_RSP_MC_STORED:
                case MSG_RSP_MC_NOT_STORED:
                case MSG_RSP_MC_EXISTS:
                case MSG_RSP_MC_NOT_FOUND:
                case MSG_RSP_MC_DELETED:
                    state = SW_CRLF;
                    break;

                case MSG_RSP_MC_END:
                    state = SW_CRLF;
                    break;

                case MSG_RSP_MC_VALUE:
                    state = SW_SPACES_BEFORE_KEY;
                    break;

                case MSG_RSP_MC_ERROR:
                    state = SW_CRLF;
                    break;

                case MSG_RSP_MC_CLIENT_ERROR:
                case MSG_RSP_MC_SERVER_ERROR:
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
                state = SW_KEY;
                p = p - 1; /* go back by 1 byte */
            }

            break;

        case SW_KEY:
            if (ch == ' ') {
                r->key_end = p;
                /* r->token = NULL; */
                state = SW_SPACES_BEFORE_FLAGS;
            }

            break;

        case SW_SPACES_BEFORE_FLAGS:
            if (ch != ' ') {
                if (!isdigit(ch)) {
                    goto error;
                }
                state = SW_FLAGS;
                p = p - 1; /* go back by 1 byte */
            }

            break;

        case SW_FLAGS:
            if (r->token == NULL) {
                /* flags_start <- p */
                /* r->token = p; */
            }

            if (isdigit(ch)) {
                /* flags <- flags * 10 + (ch - '0') */
                ;
            } else if (ch == ' ') {
                /* flags_end <- p - 1 */
                /* r->token = NULL; */
                state = SW_SPACES_BEFORE_VLEN;
            } else {
                goto error;
            }

            break;

        case SW_SPACES_BEFORE_VLEN:
            if (ch != ' ') {
                if (!isdigit(ch)) {
                    goto error;
                }
                p = p - 1; /* go back by 1 byte */
                state = SW_VLEN;
                r->vlen = 0;
            }

            break;

        case SW_VLEN:
            if (isdigit(ch)) {
                r->vlen = r->vlen * 10 + (uint32_t)(ch - '0');
            } else if (ch == ' ' || ch == CR) {
                /* vlen_end <- p - 1 */
                p = p - 1; /* go back by 1 byte */
                /* r->token = NULL; */
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
                r->token = NULL;
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
                state = SW_VAL_LF;
                break;

            default:
                goto error;
            }

            break;

        case SW_VAL_LF:
            switch (ch) {
            case LF:
                /* state = SW_END; */
                state = SW_RSP_STR;
                break;

            default:
                goto error;
            }

            break;

        case SW_END:
            if (r->token == NULL) {
                if (ch != 'E') {
                    goto error;
                }
                /* end_start <- p */
                r->token = p;
            } else if (ch == CR) {
                /* end_end <- p */
                m = r->token;
                r->token = NULL;

                switch (p - m) {
                case 3:
                    if (str4cmp(m, 'E', 'N', 'D', '\r')) {
                        r->end = m;
                        state = SW_ALMOST_DONE;
                    }
                    break;

                default:
                    goto error;
                }
            }

            break;

        case SW_RUNTO_CRLF:
            switch (ch) {
            case CR:
                if (r->type == MSG_RSP_MC_VALUE) {
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
        if (state <= SW_RUNTO_VAL || state == SW_CRLF || state == SW_ALMOST_DONE) {
            r->state = SW_START;
        }
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

/*
 * Pre-split copy handler invoked when the request is a multi vector -
 * 'get' or 'gets' request and is about to be split into two requests
 */
void
memcache_pre_splitcopy(struct mbuf *mbuf, void *arg)
{
    struct msg *r = arg;                  /* request vector */
    struct string get = string("get ");   /* 'get ' string */
    struct string gets = string("gets "); /* 'gets ' string */

    ASSERT(r->request);
    ASSERT(!r->redis);
    ASSERT(mbuf_empty(mbuf));

    switch (r->type) {
    case MSG_REQ_MC_GET:
        mbuf_copy(mbuf, get.data, get.len);
        break;

    case MSG_REQ_MC_GETS:
        mbuf_copy(mbuf, gets.data, gets.len);
        break;

    default:
        NOT_REACHED();
    }
}

/*
 * Post-split copy handler invoked when the request is a multi vector -
 * 'get' or 'gets' request and has already been split into two requests
 */
rstatus_t
memcache_post_splitcopy(struct msg *r)
{
    struct mbuf *mbuf;
    struct string crlf = string(CRLF);

    ASSERT(r->request);
    ASSERT(!r->redis);
    ASSERT(!STAILQ_EMPTY(&r->mhdr));

    mbuf = STAILQ_LAST(&r->mhdr, mbuf, next);
    mbuf_copy(mbuf, crlf.data, crlf.len);

    return NC_OK;
}

/*
 * parse next key in mget request, update:
 *   r->key_start
 *   r->key_end
 * */
static rstatus_t
memcache_retrieval_update_keypos(struct msg *r)
{
    struct mbuf *mbuf;
    uint8_t *p;

    for (mbuf = STAILQ_FIRST(&r->mhdr);
         mbuf && (mbuf->pos >= mbuf->last);
         mbuf = STAILQ_FIRST(&r->mhdr)) {

        mbuf_remove(&r->mhdr, mbuf);
        mbuf_put(mbuf);
    }

    p = mbuf->pos;
    for (; p < mbuf->last && isspace(*p); p++) {
        /* eat spaces */
    }

    r->key_start = p;
    for (; p < mbuf->last && !isspace(*p); p++) {
        /* read key */
    }
    r->key_end = p;

    for (; p < mbuf->last && isspace(*p); p++) {
        /* eat spaces */
    }
    mbuf->pos = p;

    return NC_OK;
}

static rstatus_t
memcache_append_key(struct msg *r, uint8_t *key, uint32_t keylen)
{
    struct mbuf *mbuf;

    mbuf = msg_ensure_mbuf(r, keylen + 2);
    if (mbuf == NULL) {
        nc_free(r);
        return NC_ENOMEM;
    }
    r->key_start = mbuf->last;   /* update key_start */
    mbuf_copy(mbuf, key, keylen);
    r->mlen += keylen;
    r->key_end = mbuf->last;     /* update key_start */

    mbuf_copy(mbuf, (uint8_t *)" ", 1);
    r->mlen += 1;
    return NC_OK;
}

static rstatus_t
memcache_fragment_retrieval(struct msg *r, uint32_t ncontinuum,
                            struct msg_tqh *frag_msgq,
                            uint32_t key_step)
{
    struct mbuf *mbuf, *sub_msg_mbuf;
    struct msg **sub_msgs;
    uint32_t i;

    /* init sub_msgs and r->frag_seq */
    sub_msgs = nc_zalloc(ncontinuum * sizeof(void *));

    /* the point for each key, point to sub_msgs elements */
    r->frag_seq = nc_alloc(sizeof(struct msg *) * r->narg);

    mbuf = STAILQ_FIRST(&r->mhdr);
    mbuf->pos = mbuf->start;

    for (; *(mbuf->pos) != ' ';) { /* eat get/gets  */
        mbuf->pos++;
    }
    mbuf->pos++;

    r->frag_id = msg_gen_frag_id();
    r->first_fragment = 1;
    r->nfrag = 0;
    r->frag_owner = r;

    for (i = 1; i < r->narg; i++) {        /* for each  key */
        uint8_t *key;
        uint32_t keylen;
        uint32_t idx;
        struct msg *sub_msg;

        memcache_retrieval_update_keypos(r);
        key = r->key_start;
        keylen = (uint32_t)(r->key_end - r->key_start);
        idx = msg_backend_idx(r);

        if (sub_msgs[idx] == NULL) {
            sub_msgs[idx] = msg_get(r->owner, r->request, r->redis);
        }
        sub_msg = sub_msgs[idx];

        r->frag_seq[i] = sub_msgs[idx];

        sub_msg->narg++;
        if (NC_OK != memcache_append_key(sub_msg, key, keylen)) {
            nc_free(sub_msgs);
            return NC_ENOMEM;
        }
    }

    for (mbuf = STAILQ_FIRST(&r->mhdr);
         mbuf && (mbuf->pos >= mbuf->last);
         mbuf = STAILQ_FIRST(&r->mhdr)) {

        mbuf_remove(&r->mhdr, mbuf);
        mbuf_put(mbuf);
    }
    ASSERT(STAILQ_EMPTY(&r->mhdr));

    for (i = 0; i < ncontinuum; i++) {     /* prepend mget header, and forward it */
        struct msg *sub_msg = sub_msgs[i];
        if (sub_msg == NULL) {
            continue;
        }

        /* prepend get/gets (TODO: use a function) */
        sub_msg_mbuf = mbuf_get();
        if (sub_msg_mbuf == NULL) {
            nc_free(sub_msgs);
            return NC_ENOMEM;
        }
        if (r->type == MSG_REQ_MC_GET) {
            sub_msg_mbuf->last +=
                nc_snprintf(sub_msg_mbuf->last, mbuf_size(sub_msg_mbuf), "get ");
        } else if (r->type == MSG_REQ_MC_GETS) {
            sub_msg_mbuf->last +=
                nc_snprintf(sub_msg_mbuf->last, mbuf_size(sub_msg_mbuf), "gets ");
        }
        sub_msg->mlen += mbuf_length(sub_msg_mbuf);
        STAILQ_INSERT_HEAD(&sub_msg->mhdr, sub_msg_mbuf, next);

        /* append \r\n */
        sub_msg_mbuf = mbuf_get();
        if (sub_msg_mbuf == NULL) {
            nc_free(sub_msgs);
            return NC_ENOMEM;
        }
        sub_msg_mbuf->last +=
            nc_snprintf(sub_msg_mbuf->last, mbuf_size(sub_msg_mbuf), "\r\n");
        sub_msg->mlen += mbuf_length(sub_msg_mbuf);
        STAILQ_INSERT_TAIL(&sub_msg->mhdr, sub_msg_mbuf, next);

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
memcache_fragment(struct msg *r, uint32_t ncontinuum, struct msg_tqh *frag_msgq)
{
    if (memcache_retrieval(r)) {
        rstatus_t status = memcache_fragment_retrieval(r, ncontinuum, frag_msgq, 1);
        if (status != NC_OK) {
            return status;
        }
    }
    return NC_OK;
}

/*
 * Pre-coalesce handler is invoked when the message is a response to
 * the fragmented multi vector request - 'get' or 'gets' and all the
 * responses to the fragmented request vector hasn't been received
 */
void
memcache_pre_coalesce(struct msg *r)
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

    case MSG_RSP_MC_VALUE:
    case MSG_RSP_MC_END:

        /*
         * Readjust responses of the fragmented message vector by not
         * including the end marker for all but the last response
         */

        if (pr->last_fragment) {
            break;
        }

        ASSERT(r->end != NULL);

        for (;;) {
            mbuf = STAILQ_LAST(&r->mhdr, mbuf, next);
            ASSERT(mbuf != NULL);

            /*
             * We cannot assert that end marker points to the last mbuf
             * Consider a scenario where end marker points to the
             * penultimate mbuf and the last mbuf only contains spaces
             * and CRLF: mhdr -> [...END] -> [\r\n]
             */

            if (r->end >= mbuf->pos && r->end < mbuf->last) {
                /* end marker is within this mbuf */
                r->mlen -= (uint32_t)(mbuf->last - r->end);
                mbuf->last = r->end;
                break;
            }

            /* end marker is not in this mbuf */
            r->mlen -= mbuf_length(mbuf);
            mbuf_remove(&r->mhdr, mbuf);
            mbuf_put(mbuf);
        }

        break;

    default:
        /*
         * Valid responses for a fragmented requests are MSG_RSP_MC_VALUE or,
         * MSG_RSP_MC_END. For an invalid response, we send out SERVER_ERRROR
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

/*
 * copy one response from src to dst
 * return bytes copied
 * */
uint32_t
memcache_copy_bulk(struct msg *dst, struct msg *src)
{
    struct mbuf *mbuf, *nbuf;
    uint8_t *p;
    uint32_t len = 0;
    uint32_t bytes = 0;
    uint32_t i = 0;

    for (mbuf = STAILQ_FIRST(&src->mhdr);
         mbuf && (mbuf->pos >= mbuf->last);
         mbuf = STAILQ_FIRST(&src->mhdr)) {

        mbuf_remove(&src->mhdr, mbuf);
        mbuf_put(mbuf);
    }

    mbuf = STAILQ_FIRST(&src->mhdr);
    if (mbuf == NULL) {
        return 0;
    }
    p = mbuf->pos;

    /* get : VALUE key 0 len\r\nv\r\n */
    /* gets: VALUE key 0 len cas\r\rnv\r\n */

    ASSERT(*p == 'V');
    for (i = 0; i < 3; i++) {                 /*  eat 'VALUE key 0 '  */
        for (; *p != ' ';) {
            p++;
        }
        p++;
    }

    len = 0;
    for (; p < mbuf->last && isdigit(*p); p++) {
        len = len * 10 + (uint32_t)(*p - '0');
    }

    for (; p < mbuf->last && ('\r' != *p); p++) { /* eat cas for gets */
        ;
    }

    len += CRLF_LEN * 2;
    len += (p - mbuf->pos);

    bytes = len;

    /* copy len bytes to dst */
    for (; mbuf;) {
        if (mbuf_length(mbuf) <= len) {   /* steal this mbuf from src to dst */
            nbuf = STAILQ_NEXT(mbuf, next);
            mbuf_remove(&src->mhdr, mbuf);
            mbuf_insert(&dst->mhdr, mbuf);
            len -= mbuf_length(mbuf);
            mbuf = nbuf;
        } else {                        /* split it */
            nbuf = mbuf_get();
            if (nbuf == NULL) {
                return -1;
            }
            mbuf_copy(nbuf, mbuf->pos, len);
            mbuf_insert(&dst->mhdr, nbuf);
            mbuf->pos += len;
            break;
        }
    }
    return bytes;
}

/*
 * Post-coalesce handler is invoked when the message is a response to
 * the fragmented multi vector request - 'get' or 'gets' and all the
 * responses to the fragmented request vector has been received and
 * the fragmented request is consider to be done
 */
void
memcache_post_coalesce(struct msg *request)
{
    struct msg *response = request->peer;
    struct mbuf *mbuf;
    uint32_t len;
    struct msg *sub_msg;
    uint32_t i;

    ASSERT(!response->request);
    ASSERT(request->request && request->first_fragment);
    if (request->error || request->ferror) {
        /* do nothing, if msg is in error */
        return;
    }

    for (i = 1; i < request->narg; i++) {               /* for each  key */
        sub_msg = request->frag_seq[i]->peer;           /* get it's peer response */
        if (sub_msg == NULL) {
            /*
             * no response because of error, we do nothing and leave it to the
             * req_error() check in rsp_send_next
             */
            continue;
        }
        len = memcache_copy_bulk(response, sub_msg);
        ASSERT(len >= 0);
        log_debug(LOG_VVERB, "memcache_copy_bulk for mget copy bytes: %d", len);
        response->mlen += len;
        sub_msg->mlen -= len;
    }

    /* append END\r\n */
    mbuf = msg_ensure_mbuf(response, 5);
    if (mbuf == NULL) {
        return;
    }
    len = nc_snprintf(mbuf->last, mbuf_size(mbuf), "END\r\n");
    mbuf->last += len;
    response->mlen += len;

    nc_free(request->frag_seq);
}
