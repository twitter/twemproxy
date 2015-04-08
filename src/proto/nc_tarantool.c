/*
 * Tarantool support for twemproxy.
 * Copyright (C) 2014 Alexey Kopytov
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
#include <nc_proto.h>
#include <nc_message.h>
#include <nc_prng.h>

#define MP_SOURCE 1
#include "msgpuck.h"

#include "base64.h"

#define IPROTO_GREETING_SIZE 128
#define IPROTO_SALT_SIZE 32

/* Supported request types */
typedef enum tarantool_request_code {
    TARANTOOL_OK      = 0,
    TARANTOOL_SELECT  = 1,
    TARANTOOL_INSERT  = 2,
    TARANTOOL_REPLACE = 3,
    TARANTOOL_UPDATE  = 4,
    TARANTOOL_DELETE  = 5,
    TARANTOOL_CALL    = 6,
    TARANTOOL_AUTH    = 7,
    TARANTOOL_PING    = 64
} tarantool_request_code_t;

/* Supported keys */
typedef enum iproto_key {
    IPROTO_CODE = 0x00,
    IPROTO_SYNC = 0x01,
    IPROTO_SPACE_ID = 0x10,
    IPROTO_INDEX_ID = 0x11,
    IPROTO_LIMIT = 0x12,
    IPROTO_OFFSET = 0x13,
    IPROTO_ITERATOR = 0x14,
    IPROTO_KEY = 0x20,
    IPROTO_TUPLE = 0x21,
    IPROTO_FUNCTION_NAME = 0x22,
    IPROTO_USER_NAME = 0x23,
    IPROTO_DATA = 0x30,
    IPROTO_ERROR = 0x31
} iproto_key_t;

static inline msg_type_t
tarantool_code_to_msg_type(uint32_t code)
{
    switch ((tarantool_request_code_t)code) {
    case TARANTOOL_OK:
        return MSG_RSP_TARANTOOL_OK;
    case TARANTOOL_SELECT:
        return MSG_REQ_TARANTOOL_SELECT;
    case TARANTOOL_INSERT:
        return MSG_REQ_TARANTOOL_INSERT;
    case TARANTOOL_REPLACE:
        return MSG_REQ_TARANTOOL_REPLACE;
    case TARANTOOL_UPDATE:
        return MSG_REQ_TARANTOOL_UPDATE;
    case TARANTOOL_DELETE:
        return MSG_REQ_TARANTOOL_DELETE;
    case TARANTOOL_CALL:
        return MSG_REQ_TARANTOOL_CALL;
    case TARANTOOL_AUTH:
        return MSG_REQ_TARANTOOL_AUTH;
    case TARANTOOL_PING:
        return MSG_REQ_TARANTOOL_PING;
    default:
        if ((code & 0x8000U) == 0x8000U) {
            return MSG_RSP_TARANTOOL_ERROR;
        }

        break;
    }

    return MSG_UNKNOWN;
}

/*
 * Return true, if iproto request header is parsed successfully, otherwise
 * return false.
 */
static bool
tarantool_parse_header(struct msg *r, const char **pos, const char *end)
{
    const char *p;
    uint32_t size, i;

    ASSERT(*pos < end);

    p = *pos;

    if (mp_typeof(*p) != MP_MAP || mp_check(&p, end) != 0) {
        return false;
    }

    p = *pos;

    size = mp_decode_map(&p);

    for (i = 0; i < size; i++) {
        unsigned char key;

        if (mp_typeof(*p) != MP_UINT) {
            return false;
        }

        key = mp_decode_uint(&p);
        switch (key) {
        case IPROTO_CODE:
            r->code = mp_decode_uint(&p);
            r->type = tarantool_code_to_msg_type(r->code);

            if (r->type == MSG_UNKNOWN) {
                return false;
            }

            break;

        case IPROTO_SYNC:
            r->sync = mp_decode_uint(&p);

            break;

        default:
            mp_next(&p);
            break;
        }
    }

    *pos = p;

    return true;
}

/*
 * Return true, if a given key value is parsed successfully, otherwise return
 * false.
 */
static bool
tarantool_parse_key(struct msg *r, const char *pos, const char *end)
{
    uint32_t part_count;

    ASSERT(pos < end);
    ASSERT(r->body_start != NULL);
    ASSERT(r->body_start < (uint8_t *)pos);

    if (mp_typeof(*pos) != MP_ARRAY) {
        return false;
    }

    part_count = mp_decode_array(&pos);

    if (part_count == 0) {
        r->key_offset = (uint8_t *)pos - r->body_start;
        r->key_len = 0;
        return true;
    }

    /*
     * Currently only the first part of a multi-part key is considered a
     * sharding key.
     */
    switch (mp_typeof(*pos)) {
    case MP_STR:
        r->key_offset = (uint8_t *)mp_decode_str(&pos, &r->key_len) -
            r->body_start;
        break;
    case MP_BIN:
        r->key_offset = (uint8_t *)mp_decode_bin(&pos, &r->key_len) -
            r->body_start;
        break;
    default:
        r->key_offset = (uint8_t *)pos - r->body_start;
        mp_next(&pos);
        r->key_len = (uint8_t *)pos - r->body_start - r->key_offset;

        break;
    }

    return true;

}

/*
 * Return true, if iproto request body is parsed successfully, otherwise
 * return false.
 */
static bool
tarantool_parse_body(struct msg *r, const char **pos, const char *end)
{
    const char *p;
    uint32_t size, i;

    ASSERT(*pos < end);

    p = *pos;
    r->body_start = (uint8_t *)p;

    if (mp_typeof(*p) != MP_MAP || mp_check(&p, end) != 0) {
        return false;
    }

    p = *pos;

    size = mp_decode_map(&p);

    for (i = 0; i < size; i++) {
        unsigned char key;
        const char *value;

        if (mp_typeof(*p) != MP_UINT) {
            return false;
        }

        key = mp_decode_uint(&p);
        value = p;

        if (mp_check(&p, end) != 0) {
            return false;
        }

        /*
         * For SELECT/UPDATE/DELETE iproto key is the sharding key.
         * For INSERT/REPLACE/CALL iproto tuple is the sharding key.
         */
        switch (key) {
        case IPROTO_KEY:
            if (!tarantool_parse_key(r, value, end)) {
                return false;
            }
            break;
        case IPROTO_TUPLE:
            if ((r->type == MSG_REQ_TARANTOOL_INSERT ||
                 r->type == MSG_REQ_TARANTOOL_REPLACE ||
                 r->type == MSG_REQ_TARANTOOL_CALL) &&
                !tarantool_parse_key(r, value, end)) {
                return false;
            }
            break;
        default:
            break;
        }
    }

    *pos = p;

    return true;
}

/*
 * Reference: http://tarantool.org/doc/box-protocol.html
 *
 * twemproxy only supports the MessagePack-based protocol introduced in
 * Tarantool 1.6.
 *
 * Unified request format:
 *
 * 0      5                  HEADER                            BODY
 * +------+ +==================+==================+ +=========================+
 * |BODY +| |                  |                  | |                         |
 * |HEADER| |   0x00:  CODE    |   0x01: SYNC     | |                         |
 * |      | | MP_UINT: MP_UINT | MP_UINT: MP_UINT | |                         |
 * | SIZE | |                  |                  | |                         |
 * +------+ +==================+==================+ +=========================+
 *  MP_UINT                  MP_MAP                            MP_MAP
 *
 * Notes:
 *
 * 1. the protocol specification mandates use of MP_INT values in all header
 * fields, though in fact all implementations encode and expect MP_UINT values.
 *
 * 2. the protocol specification sounds like the first field "body + header
 * size" is fixed length 5-byte value (MP_UINT32). The server actually expects a
 * variable length MP_UINT value which can take 1-9 bytes.
 */
void
tarantool_parse_req(struct msg *r)
{
    struct mbuf *b;
    uint8_t *p, *m;
    const char *pos;

    b = STAILQ_LAST(&r->mhdr, mbuf, next);

    ASSERT(r->request);
    ASSERT(r->proto == PROTO_TARANTOOL);
    ASSERT(b != NULL);
    ASSERT(b->pos <= b->last);

    /* validate the parsing marker */
    ASSERT(r->pos != NULL);
    ASSERT(r->pos >= b->pos && r->pos <= b->last);

    p = r->pos;

    r->token = p;

    if (mp_typeof(*p) != MP_UINT) {
        goto error;
    }

    if (mp_check_uint((const char *)r->token, (const char *)b->last) > 0) {
        p = b->last;
        goto needmore;
    }

    r->reqlen = mp_decode_uint((const char **)&p);

    m = p + r->reqlen;

    if (m > b->last) {
        p = b->last;
        goto needmore;
    }

    pos = (const char *)p;

    if (!tarantool_parse_header(r, &pos, (const char *)m)) {
        goto error;
    }

    if ((uint8_t *)pos < m &&
        !tarantool_parse_body(r, &pos, (const char *)m)) {
        goto error;
    }

    ASSERT(pos == (const char *)m);

    p = m - 1;

    switch (r->type) {
    case MSG_REQ_TARANTOOL_AUTH:
    case MSG_REQ_TARANTOOL_PING:
        r->noforward = 1;
        goto done;

    default:
        break;
    }

    goto done;

needmore:
    ASSERT(p == b->last);
    r->pos = r->token;

    if (b->last == b->end && r->token != NULL) {
        r->token = NULL;
        r->result = MSG_PARSE_REPAIR;
    } else {
        r->result = MSG_PARSE_AGAIN;
    }

    log_hexdump(LOG_VERB, b->pos, mbuf_length(b), "parsed req %"PRIu64" res %d "
                "type %d state %d rpos %d of %d", r->id, r->result, r->type,
                r->state, r->pos - b->pos, b->last - b->pos);

    return;

done:
    ASSERT(r->type > MSG_UNKNOWN && r->type < MSG_SENTINEL);
    r->pos = p + 1;
    ASSERT(r->pos <= b->last);
    r->token = NULL;
    r->result = MSG_PARSE_OK;

    log_hexdump(LOG_VERB, b->pos, mbuf_length(b), "parsed req %"PRIu64" res %d "
                "type %d state %d rpos %d of %d", r->id, r->result, r->type,
                r->state, r->pos - b->pos, b->last - b->pos);
    return;

error:
    r->result = MSG_PARSE_ERROR;
    errno = EINVAL;

    log_hexdump(LOG_INFO, b->pos, mbuf_length(b), "parsed bad req %"PRIu64" "
                "res %d type %d state %d", r->id, r->result, r->type,
                r->state);
}

void tarantool_parse_rsp(struct msg *r)
{
    struct mbuf *b;
    uint8_t *p, *m;
    const char *pos, *end;
    int i;
    bool morebytes;
    uint32_t size;

    enum {
        SW_START,
        SW_GREETING,
        SW_RESP_LEN,
        SW_RESP_HEADER,
        SW_RESP_BODY,
        SW_SENTINEL
    } state;

    state = r->state;
    b = STAILQ_LAST(&r->mhdr, mbuf, next);

    ASSERT(!r->request);
    ASSERT(r->proto == PROTO_TARANTOOL);
    ASSERT(state >= SW_START && state < SW_SENTINEL);
    ASSERT(b != NULL);
    ASSERT(b->pos <= b->last);

    /* validate the parsing marker */
    ASSERT(r->pos != NULL);
    ASSERT(r->pos >= b->pos && r->pos <= b->last);

    /*
     * Check if it is the first message from the server. In which case it must
     * be a greeting message. We ignore it by eating 128 bytes.
     * TODO: support authentication by parsing the salt value
     */
    if (r->owner->greeting) {
        ASSERT(!r->owner->client && !r->owner->proxy);
        ASSERT(state == SW_START);

        state = SW_GREETING;
        r->owner->greeting = 0;
    }

    for (p = r->pos; p < b->last; p++) {

        switch (state) {

        case SW_START:

            if (mp_typeof(*p) != MP_UINT) {
                goto error;
            }

            r->token = p;

            state = SW_RESP_LEN;

            break;

        case SW_GREETING:

            if (r->token == NULL) {
                r->token = p;
            }

            m = r->token + IPROTO_GREETING_SIZE;

            if (m > b->last) {
                p = b->last - 1;
                break;
            }

            r->token = NULL;
            state = SW_START;
            p = m - 1;

            break;

        case SW_RESP_LEN:

            ASSERT(r->token != NULL);

            if (mp_check_uint((const char *)r->token,
                              (const char *)b->last) > 0) {
                p = b->last - 1;
                break;
            }

            p = r->token;

            r->reqlen = mp_decode_uint((const char **)&p);

            r->token = NULL;
            p--;

            state = SW_RESP_HEADER;

            break;

        case SW_RESP_HEADER:

            pos = (const char *)p;
            end = (const char *)b->last;

            /* Is it a map? */
            if (mp_typeof(*pos) != MP_MAP) {
                goto error;
            }

            /* Need more bytes? */
            if (mp_check_map(pos, end) > 0) {
                p = b->last - 1;
                break;
            }

            size = mp_decode_map(&pos);

            morebytes = false;

            for (i = 0; i < size * 2; i++) {
                if (pos >= end) {
                    morebytes = true;
                    break;
                }

                if (mp_typeof(*pos) != MP_UINT) {
                    goto error;
                }

                if (mp_check(&pos, end)) {
                    morebytes = true;
                    break;
                }
            }

            if (morebytes) {
                p = b->last - 1;
                break;
            }

            pos = (const char *)p;
            if (!tarantool_parse_header(r, &pos, end)) {
                goto error;
            }

            ASSERT(pos - (const char *)p <= r->reqlen);

            r->reqlen -= pos - (const char *)p;

            p = (uint8_t *)pos - 1;

            if (r->reqlen == 0) {
                goto done;
            }

            state = SW_RESP_BODY;

            break;

        case SW_RESP_BODY:

            if (r->body_start == NULL) {
                r->body_start = p;
            }

            /* Just read response body without parsing it */
            if (b->last - p < r->reqlen) {
                r->reqlen -= b->last - p;
                p = b->last - 1;
                break;
            }

            p += r->reqlen - 1;
            r->reqlen = 0;

            goto done;

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

/*
 * Emit a pseudo 'connect' message on a client connect to respond with a
 * greeting message. This is a workaround for the twemproxy architecture which
 * is tightly dependent on pure request/response protocols.
 */
void
tarantool_post_connect(struct context *ctx, struct conn *conn)
{
    struct msg *msg;

    if (!conn->client) {
        return;
    }

    ASSERT(!conn->proxy);
    ASSERT(conn->proto = PROTO_TARANTOOL);
    ASSERT(conn->rmsg == NULL);

    msg = msg_get(conn, true, conn->proto);
    if (msg == NULL) {
        return;
    }

    msg->type = MSG_REQ_CONNECT;
    msg->result = MSG_PARSE_OK;
    msg->owner = conn;

    /* Reply without forwarding to a server */
    msg->noforward = 1;

    /* Prevent empty message filtering */
    msg->emitted = 1;

    conn->rmsg = msg;

    conn->recv_done(ctx, conn, msg, NULL);

    log_debug(LOG_NOTICE, "sent greeting msg to c %d from '%s'", conn->sd,
              nc_unresolve_peer_desc(conn->sd));

}

static size_t
tarantool_sizeof_header(uint32_t code, uint32_t sync)
{
    return 5 +
        mp_sizeof_map(2) +
        mp_sizeof_uint(IPROTO_CODE) +
        mp_sizeof_uint(code) +
        mp_sizeof_uint(IPROTO_SYNC) +
        mp_sizeof_uint(sync);
}

static void
tarantool_encode_header(uint8_t **ptr, size_t size, uint32_t code,
                        uint32_t sync)
{
    char *p;

    ASSERT(size > 5);

    p = (char *)*ptr;

    /* size */
    *(uint8_t *)p++ = 0xce;
    *(uint32_t *)p = mp_bswap_u32(size - 5);
    p += 4;

    /* map(2) */
    p = mp_encode_map(p, 2);

    /* code */
    p = mp_encode_uint(p, IPROTO_CODE);
    p = mp_encode_uint(p, code);

    /* sync */
    p = mp_encode_uint(p, IPROTO_SYNC);
    p = mp_encode_uint(p, sync);

    *ptr = (uint8_t *)p;
}

rstatus_t tarantool_fragment(struct msg *r, uint32_t ncontinuum,
                             struct msg_tqh *frag_msgq)
{
    struct msg *frag_msg;
    struct mbuf *frag_mbuf;
    struct mbuf *mbuf;
    struct keypos *kpos;
    size_t size;

    /*
     * The Tarantool protocol supports out-of-order responses via the 'sync'
     * field. It is a mandatory field in a message which is used to match a
     * response to a request. Since twemproxy can multiplex client connections
     * to server ones, the sync field in request headers has to be rewritten to
     * make it unique across multiple clients talking to the same server.
     *
     * We achieve this by 'fragmenting' every client request into exactly one
     * request to the server. The fragment duplicates the original request body,
     * but gets a new header with the sync field rewritten.
     */

    ASSERT(r->request);
    ASSERT(r->key_offset > 0);
    ASSERT(r->body_start != NULL);
    ASSERT(r->frag_seq == NULL);

    r->frag_seq = nc_alloc(sizeof(*r->frag_seq));
    if (r->frag_seq == NULL) {
        return NC_ENOMEM;
    }

    r->frag_id = msg_gen_frag_id();
    r->nfrag = 1;
    r->frag_owner = r;

    mbuf = STAILQ_FIRST(&r->mhdr);

    ASSERT(mbuf != NULL);
    ASSERT(r->body_start >= mbuf->start && r->body_start < mbuf->last);

    frag_msg = msg_get(r->owner, true, r->proto);
    if (frag_msg == NULL) {
        return NC_ENOMEM;
    }

    r->frag_seq[0] = frag_msg;

    frag_msg->type = r->type;
    frag_msg->frag_id = r->frag_id;
    frag_msg->frag_owner = r->frag_owner;
    /* Use r->frag_id as the new sync value */
    frag_msg->sync = r->frag_id;

    size = tarantool_sizeof_header(r->code, frag_msg->sync) +
        (mbuf->last - r->body_start);
    frag_mbuf = msg_ensure_mbuf(frag_msg, size);
    if (frag_mbuf == NULL) {
        return NC_ENOMEM;
    }

    tarantool_encode_header(&frag_mbuf->last, size, r->code, frag_msg->sync);

    /*
     * Set up frag_msg->keys based on the offset and len from the original
     * request
     */
    kpos = array_push(frag_msg->keys);
    if (kpos == NULL) {
        return NC_ENOMEM;
    }

    kpos->start = frag_mbuf->last + r->key_offset;
    kpos->end = kpos->start + r->key_len;

    /*
     * Copy body from the original request. Each client request is guaranteed to
     * fit in a single mbuf and thus, use a contiguous memory region.
     */
    mbuf_copy(frag_mbuf, r->body_start, mbuf->last - r->body_start);

    frag_msg->mlen += size;

    TAILQ_INSERT_TAIL(frag_msgq, frag_msg, m_tqe);

    return NC_OK;
}

static rstatus_t
tarantool_get_greeting(struct msg *msg)
{
    int n;
    struct mbuf *mbuf;
    char salt[IPROTO_SALT_SIZE];
    char base64buf[IPROTO_SALT_SIZE * 4 / 3 + 5];

    mbuf = msg_ensure_mbuf(msg, IPROTO_GREETING_SIZE + 1);
    if (mbuf == NULL) {
        return NC_ENOMEM;
    }

    nc_prng_fill(salt, IPROTO_SALT_SIZE);

    base64_encode(salt, IPROTO_SALT_SIZE, base64buf, sizeof(base64buf));

    n = nc_scnprintf(mbuf->last, IPROTO_GREETING_SIZE + 1,
                     "Tarantool %-20s %-32s\n%-63s\n",
                     "1.6", "("PACKAGE_STRING")", base64buf);

    mbuf->last += n;
    msg->mlen += (uint32_t)n;

    return NC_OK;
}

static rstatus_t
tarantool_get_pong(struct msg *msg)
{
    struct msg *r;
    struct mbuf *mbuf;
    size_t size;

    r = msg->peer;

    ASSERT(r != NULL);
    ASSERT(r->type == MSG_REQ_TARANTOOL_PING);

    /* Reply with an empty OK packet to a PING request */

    size = tarantool_sizeof_header(TARANTOOL_OK, r->sync);

    mbuf = msg_ensure_mbuf(msg, size);
    if (mbuf == NULL) {
        return NC_ENOMEM;
    }

    tarantool_encode_header(&mbuf->last, size, TARANTOOL_OK, r->sync);

    msg->mlen += size;

    log_hexdump(LOG_VERB, mbuf->pos, mbuf_length(mbuf),
                "sending pong msg %"PRIu64" len %u", msg->id, msg->mlen);

    return NC_OK;
}

static rstatus_t
tarantool_get_ok(struct msg *msg)
{
    struct msg *r;
    struct mbuf *mbuf;
    size_t size;

    r = msg->peer;

    ASSERT(r != NULL);
    ASSERT(r->type == MSG_REQ_TARANTOOL_AUTH);

    /* Reply with an empty OK packet to an auth request */

    size = tarantool_sizeof_header(TARANTOOL_OK, r->sync);

    mbuf = msg_ensure_mbuf(msg, size);
    if (mbuf == NULL) {
        return NC_ENOMEM;
    }

    tarantool_encode_header(&mbuf->last, size, TARANTOOL_OK, r->sync);

    msg->mlen += size;

    log_hexdump(LOG_VERB, mbuf->pos, mbuf_length(mbuf),
                "sending ok msg %"PRIu64" len %u", msg->id, msg->mlen);

    return NC_OK;
}

rstatus_t
tarantool_reply(struct msg *r)
{
    struct msg *response = r->peer;

    ASSERT(response != NULL);

    switch (r->type) {
    case MSG_REQ_CONNECT:
        return tarantool_get_greeting(response);

    case MSG_REQ_TARANTOOL_PING:
        return tarantool_get_pong(response);

    case MSG_REQ_TARANTOOL_AUTH:
        return tarantool_get_ok(response);

    default:
        NOT_REACHED();
        return NC_ERROR;
    }
}

void
tarantool_get_error(struct msg *r, struct msg *msg, const char *errstr)
{
    struct mbuf *mbuf;
    size_t size, errlen;
    char *p;
    uint32_t code = 0x8000; /* ER_UNKNOWN */

    ASSERT(msg->mlen == 0);

    errlen = strlen(errstr);

    size =
        tarantool_sizeof_header(code, r->sync) +
        mp_sizeof_map(1) +
        mp_sizeof_uint(IPROTO_ERROR) +
        mp_sizeof_str(errlen);

    mbuf = msg_ensure_mbuf(msg, size);
    if (mbuf == NULL) {
        return;
    }

    tarantool_encode_header(&mbuf->last, size, code, r->sync);

    p = (char *)mbuf->last;

    p = mp_encode_map(p, 1);
    p = mp_encode_uint(p, IPROTO_ERROR);
    p = mp_encode_str(p, errstr, errlen);

    mbuf->last = (uint8_t *)p;
    msg->mlen += size;

    msg->type = MSG_RSP_TARANTOOL_ERROR;

    log_hexdump(LOG_VERB, mbuf->pos, mbuf_length(mbuf),
                "sending error msg %"PRIu64" len %u", msg->id, msg->mlen);
}

void tarantool_pre_coalesce(struct msg *r)
{
    ASSERT(!r->request);
    ASSERT(r->peer->request);
    ASSERT(r->peer->frag_id != 0);
    ASSERT(r->peer->frag_owner->nfrag_done == 0);

    r->peer->frag_owner->nfrag_done++;
}

void tarantool_post_coalesce(struct msg *orig_req)
{
    struct msg *orig_rsp = orig_req->peer; /* peer response */
    struct msg *frag_rsp;
    size_t header_len;
    size_t body_len;
    size_t msg_len;
    struct mbuf *mbuf;
    struct mbuf *frag_mbuf;

    ASSERT(!orig_rsp->request);
    ASSERT(orig_req->request && (orig_req->frag_owner == orig_req));
    ASSERT(orig_rsp->mlen == 0);

    if (orig_req->error || orig_req->ferror) {
        /* do nothing, if msg is in error */
        return;
    }

    ASSERT(orig_req->frag_seq != NULL);
    ASSERT(orig_req->nfrag == 1);
    ASSERT(orig_req->frag_seq[0] != NULL);

    frag_rsp = orig_req->frag_seq[0]->peer;

    if (frag_rsp == NULL) {
        orig_rsp->owner->err = 1;
        return;
    }

    /*
     * Create a new response header in orig_rsp with the original sync value and
     * copy the response body which may span multiple mbufs.
     */
    header_len = tarantool_sizeof_header(frag_rsp->code, orig_req->sync);
    mbuf = msg_ensure_mbuf(orig_rsp, header_len);
    if (mbuf == NULL) {
        orig_rsp->error = 1;
        orig_rsp->err = errno;
        return;
    }

    /* Calculate the response body length */
    frag_mbuf = STAILQ_FIRST(&frag_rsp->mhdr);

    ASSERT(frag_mbuf != NULL);
    ASSERT(frag_rsp->body_start == NULL ||
           (frag_rsp->body_start > frag_mbuf->start &&
            frag_rsp->body_start < frag_mbuf->last));

    if (frag_rsp->body_start == NULL) {
        body_len = 0;
    } else {
        body_len = frag_rsp->mlen - (frag_rsp->body_start - frag_mbuf->start);
    }

    msg_len = header_len + body_len;

    tarantool_encode_header(&mbuf->last, msg_len, frag_rsp->code,
                            orig_req->sync);

    frag_mbuf->pos = frag_rsp->body_start;

    while (frag_mbuf) {
        msg_append(orig_rsp, frag_mbuf->pos, mbuf_length(frag_mbuf));
        /*
         * Reset all 'fragment' response mbufs so that they are not sent along
         * with the 'original' response by msg_send_chain().
         */
        mbuf_rewind(frag_mbuf);
        frag_mbuf = STAILQ_NEXT(frag_mbuf, next);
    }

    frag_rsp->mlen = 0;
    orig_rsp->mlen = msg_len;
}

rstatus_t tarantool_add_auth_packet(struct context *ctx, struct conn *c_conn,
                                    struct conn *s_conn)
{
    NOT_REACHED();
    return NC_OK;
}

void tarantool_swallow_msg(struct conn *conn, struct msg *pmsg,
                           struct msg *msg)
{
}
