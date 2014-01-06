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

typedef enum {
    PROTOCOL_BINARY_REQ = 0x80,
    PROTOCOL_BINARY_RES = 0x81
} protocol_binary_magic;

typedef enum {
    PROTOCOL_BINARY_RESPONSE_SUCCESS         = 0x00,
    PROTOCOL_BINARY_RESPONSE_KEY_ENOENT      = 0x01,
    PROTOCOL_BINARY_RESPONSE_KEY_EEXISTS     = 0x02,
    PROTOCOL_BINARY_RESPONSE_E2BIG           = 0x03,
    PROTOCOL_BINARY_RESPONSE_EINVAL          = 0x04,
    PROTOCOL_BINARY_RESPONSE_NOT_STORED      = 0x05,
    PROTOCOL_BINARY_RESPONSE_DELTA_BADVAL    = 0x06,
    PROTOCOL_BINARY_RESPONSE_NOT_MY_VBUCKET  = 0x07,
    PROTOCOL_BINARY_RESPONSE_AUTH_ERROR      = 0x20,
    PROTOCOL_BINARY_RESPONSE_AUTH_CONTINUE   = 0x21,
    PROTOCOL_BINARY_RESPONSE_ERANGE          = 0x22,
    PROTOCOL_BINARY_RESPONSE_UNKNOWN_COMMAND = 0x81,
    PROTOCOL_BINARY_RESPONSE_ENOMEM          = 0x82,
    PROTOCOL_BINARY_RESPONSE_NOT_SUPPORTED   = 0x83,
    PROTOCOL_BINARY_RESPONSE_EINTERNAL       = 0x84,
    PROTOCOL_BINARY_RESPONSE_EBUSY           = 0x85,
    PROTOCOL_BINARY_RESPONSE_ETMPFAIL        = 0x86
} protocol_binary_response_status;

typedef enum {
    PROTOCOL_BINARY_CMD_GET                  = 0x00,
    PROTOCOL_BINARY_CMD_SET                  = 0x01,
    PROTOCOL_BINARY_CMD_ADD                  = 0x02,
    PROTOCOL_BINARY_CMD_REPLACE              = 0x03,
    PROTOCOL_BINARY_CMD_DELETE               = 0x04,
    PROTOCOL_BINARY_CMD_INCREMENT            = 0x05,
    PROTOCOL_BINARY_CMD_DECREMENT            = 0x06,
    PROTOCOL_BINARY_CMD_QUIT                 = 0x07,
    /*PROTOCOL_BINARY_CMD_FLUSH                = 0x08,*/
    PROTOCOL_BINARY_CMD_GETQ                 = 0x09,
    PROTOCOL_BINARY_CMD_NOOP                 = 0x0a,
    /*PROTOCOL_BINARY_CMD_VERSION              = 0x0b,*/
    PROTOCOL_BINARY_CMD_GETK                 = 0x0c,
    PROTOCOL_BINARY_CMD_GETKQ                = 0x0d,
    PROTOCOL_BINARY_CMD_APPEND               = 0x0e,
    PROTOCOL_BINARY_CMD_PREPEND              = 0x0f,
    /*PROTOCOL_BINARY_CMD_STAT                 = 0x10,*/
    PROTOCOL_BINARY_CMD_SETQ                 = 0x11,
    PROTOCOL_BINARY_CMD_ADDQ                 = 0x12,
    PROTOCOL_BINARY_CMD_REPLACEQ             = 0x13,
    PROTOCOL_BINARY_CMD_DELETEQ              = 0x14,
    PROTOCOL_BINARY_CMD_INCREMENTQ           = 0x15,
    PROTOCOL_BINARY_CMD_DECREMENTQ           = 0x16,
    PROTOCOL_BINARY_CMD_QUITQ                = 0x17,
    PROTOCOL_BINARY_CMD_FLUSHQ               = 0x18,
    PROTOCOL_BINARY_CMD_APPENDQ              = 0x19,
    PROTOCOL_BINARY_CMD_PREPENDQ             = 0x1a,
    /*PROTOCOL_BINARY_CMD_VERBOSITY            = 0x1b,*/
    PROTOCOL_BINARY_CMD_TOUCH                = 0x1c,
    PROTOCOL_BINARY_CMD_GAT                  = 0x1d,
    PROTOCOL_BINARY_CMD_GATQ                 = 0x1e,
    PROTOCOL_BINARY_CMD_GATK                 = 0x23,
    PROTOCOL_BINARY_CMD_GATKQ                = 0x24,

    PROTOCOL_BINARY_CMD_SASL_LIST_MECHS      = 0x20,
    PROTOCOL_BINARY_CMD_SASL_AUTH            = 0x21,
    PROTOCOL_BINARY_CMD_SASL_STEP            = 0x22,

    /* Range operations for GET, SET, SETQ,
     * APPEND, APPENDQ, PREPEND, PREPENDQ,
     * DELETE, DELETEQ, INCR, INCRQ, DECR and DECRQ
     * (0x30 to 0x3c) won't be supported (support
     * in memcached servers is unlikely). */

    /* VBucket management commands (GET, SET, DEL)
     * (0x3d to 0x3f) won't be supported. */

    /* TAP commands CONNECT, MUTATION, DELETE,
     * FLUSH, OPAQUE, VBUCKET_SET,
     * CHECKPOINT_START, CHECKPOINT_END
     * (0x40 to 0x47) won't be supported. */

    PROTOCOL_BINARY_CMD_LAST_RESERVED        = 0x8f

} protocol_binary_command;

typedef union {
    struct {
        uint8_t  magic;
        uint8_t  opcode;
        uint16_t keylen;
        uint8_t  extlen;
        uint8_t  datatype;
        uint16_t vbucket;
        uint32_t bodylen;
        uint32_t opaque;
        uint64_t cas;
    } request;
    struct {
        uint8_t  magic;
        uint8_t  opcode;
        uint16_t keylen;
        uint8_t  extlen;
        uint8_t  datatype;
        uint16_t status;
        uint32_t bodylen;
        uint32_t opaque;
        uint64_t cas;
    } response;
    uint8_t bytes[24];
} protocol_binary_header;

/*typedef union {
    struct {
        uint32_t flags;
        uint32_t expiration;
    } body;
    uint8_t bytes[8];
} protocol_store_req_extra;*/

/*typedef union {
    struct {
        uint64_t delta;
        uint64_t initial;
        uint32_t expiration;
    } body;
    uint8_t bytes[20];
} protocol_arithmetic_req_extras;*/

/*typedef union {
    struct {
        uint32_t flags;
    } body;
    uint8_t bytes[4];
} protocol_get_rsp_extra;*/

/*typedef union {
    struct {
        uint64_t value;
    } body;
    uint8_t bytes[8];
} protocol_arithmetic_rsp_extra;*/

typedef void (*mcdbin_parse_hdr)(struct msg *r, protocol_binary_header *hdr);
typedef void (*mcdbin_get_len)(protocol_binary_header *hdr, uint8_t *extlen,
                               uint16_t *keylen, uint32_t *valuelen);

static void
mcdbin_mbuf_parse_complete(struct msg *r, struct mbuf *b, const char *msgtype)
{

    if (b->last == b->end && r->token != NULL) {
        r->pos = r->token;
        r->token = NULL;
        r->result = MSG_PARSE_REPAIR;
    } else {
        if (r->token != NULL) {
            r->pos = r->token;
            r->token = NULL;
        }
        r->result = MSG_PARSE_AGAIN;
    }

    log_hexdump(LOG_VERB, b->pos, mbuf_length(b), "parsed %s %"PRIu64" res %d "
                "type %d state %d rpos %d of %d", msgtype, r->id, r->result,
                r->type, r->state, r->pos - b->pos, b->last - b->pos);

    return;
}

static void
mcdbin_parse_done(struct msg *r, struct mbuf *b, const char *msgtype)
{
    struct mbuf *h;

    ASSERT(r->type > MSG_UNKNOWN && r->type < MSG_SENTINEL);
    ASSERT(r->pos <= b->last);
    r->state = 0;
    r->result = MSG_PARSE_OK;

    log_hexdump(LOG_VERB, b->pos, mbuf_length(b), "parsed %s %"PRIu64" res %d "
                "type %d state %d rpos %d of %d", msgtype, r->id, r->result,
                r->type, r->state, r->pos - b->pos, b->last - b->pos);

    return;
}

static void
mcdbin_parse_error(struct msg *r, struct mbuf *b, const char *msgtype)
{
    r->result = MSG_PARSE_ERROR;
    errno = EINVAL;

    log_hexdump(LOG_INFO, b->pos, mbuf_length(b), "parsed bad %s %"PRIu64" "
                "res %d type %d state %d", msgtype, r->id, r->result, r->type,
                r->state);
    msg_dump(r);

    return;
}

static void
mcdbin_parse_packet(struct msg *r, protocol_binary_magic expected_magic,
                    mcdbin_parse_hdr hdr_parser, mcdbin_get_len len_getter,
                    bool request) {
    struct mbuf *b, *h;
    uint8_t *p;
    uint8_t ch, extlen;
    uint16_t keylen;
    uint32_t chunk_size, vlen;
    protocol_binary_header *hdr;
    const char *reqstr = request ? "req" : "rsp";
    enum state_en {
        SW_START,
        SW_PARSE_HEADER,
        SW_PARSE_EXTRAS,
        SW_PARSE_KEY,
        SW_PARSE_VALUE,
        SW_SENTINEL
    } state;

    state = r->state;
    b = STAILQ_LAST(&r->mhdr, mbuf, next);
    h = STAILQ_FIRST(&r->mhdr);

    ASSERT(r->request == request);
    ASSERT(r->protocol == MEMCACHE_BINARY);
    ASSERT(state >= SW_START && state < SW_SENTINEL);
    ASSERT(b != NULL);
    ASSERT(b->pos <= b->last);

    /* validate the parsing maker */
    ASSERT(r->pos != NULL);
    ASSERT(r->pos >= b->pos && r->pos <= b->last);

    chunk_size = 0;
    extlen = 0;
    keylen = 0;
    vlen = 0;
    for (p = r->pos; p < b->last; p += chunk_size) {
        ch = *p;

        switch (state) {
        case SW_START:
            if (ch != expected_magic) {
                goto error;
            }
            chunk_size = 0;
            state = SW_PARSE_HEADER;
            break;

        case SW_PARSE_HEADER:
            chunk_size = sizeof(*hdr);
            if (chunk_size > (uint32_t)(b->last - p)) {
                chunk_size = (uint32_t)(b->last - p);
                r->token = p;
                break;
            }
            r->token = NULL;
            hdr = (void *)p;
            len_getter(hdr, &extlen, &keylen, &vlen);
            hdr_parser(r, hdr);
            r->vlen = vlen;
            r->rlen = r->vlen;
            if (extlen > 0) {
                state = SW_PARSE_EXTRAS;
            } else if (keylen > 0) {
                state = SW_PARSE_KEY;
            } else if (vlen > 0) {
                state = SW_PARSE_VALUE;
            } else {
                goto done;
            }
            break;

        case SW_PARSE_EXTRAS:
            hdr = (void *)h->pos;
            len_getter(hdr, &extlen, &keylen, &vlen);
            chunk_size = extlen;
            if (chunk_size > (uint32_t)(b->last - p)) {
                chunk_size = (uint32_t)(b->last - p);
                r->token = p;
                break;
            }
            r->token = NULL;
            if (keylen > 0) {
                state = SW_PARSE_KEY;
            } else if (vlen > 0) {
                state = SW_PARSE_VALUE;
            } else {
                goto done;
            }
            break;

        case SW_PARSE_KEY:
            hdr = (void *)h->pos;
            len_getter(hdr, &extlen, &keylen, &vlen);
            chunk_size = keylen;
            if (chunk_size > (uint32_t)(b->last - p)) {
                chunk_size = (uint32_t)(b->last - p);
                r->token = p;
                break;
            }
            r->token = NULL;
            if (keylen > 0) {
                r->key_start = p;
                r->key_end = p + keylen;
            }
            if (keylen > MEMCACHE_MAX_KEY_LENGTH) {
                log_error("parsed bad %s %"PRIu64" of type %d with key prefix "
                          "'%.*s...' and length %d that exceeds maximum key "
                          "length", reqstr, r->id, r->type, 16, r->key_start,
                          keylen);
                goto error;
            }
            if (vlen > 0) {
                state = SW_PARSE_VALUE;
            } else {
                goto done;
            }
            break;

        case SW_PARSE_VALUE:
            r->token = NULL;
            if (r->rlen > b->last - p) {
                chunk_size = (uint32_t)(b->last - p);
                r->rlen -= chunk_size;
            } else {
                chunk_size = r->rlen;
                r->rlen -= chunk_size;
                goto done;
            }
            break;

        case SW_SENTINEL:
            NOT_REACHED();
            break;
        }
    }

    /*
     * Same as memcached ascii protocol:
     * At this point, buffer from b->pos to b->last has been parsed completely
     * but we haven't been able to reach to any conclusion. Normally, this
     * means that we have to parse again starting from the state we are in
     * after more data has been read. The newly read data is either read into
     * a new mbuf, if existing mbuf is full (b->last == b->end) or into the
     * existing mbuf
     *
     * The only exception to this is when the existing mbuf is full (b->last
     * is at b->end) and token marker is set, which means that we have to
     * copy the partial token into a new mbuf and parse again with more data
     * read into new mbuf.
     *
     * Note: This last exception should not apply unless the key overflows
     * from the current mbuf (which should not occur since header size + extra
     * lenght + key length should stay below the minimum mbuf size)
     */
    ASSERT(p == b->last);
    r->pos = p;
    r->state = state;

    mcdbin_mbuf_parse_complete(r, b, reqstr);
    return;

done:
    r->pos = p + chunk_size;
    mcdbin_parse_done(r, b, reqstr);
    return;

error:
    r->state = state;
    mcdbin_parse_error(r, b, reqstr);
    return;
}

static void
mcdbin_get_req_lenvalues(protocol_binary_header *hdr, uint8_t *extlen,
                         uint16_t *keylen, uint32_t *valuelen)
{
    if (extlen != NULL) {
        *extlen = hdr->request.extlen;
    }
    if (keylen != NULL) {
        *keylen = ntohs(hdr->request.keylen);
    }
    if (valuelen != NULL) {
        *valuelen = ntohl(hdr->request.bodylen) - ntohs(hdr->request.keylen) -
                    hdr->request.extlen;
    }
}

static void
mcdbin_parse_req_header(struct msg *r, protocol_binary_header *hdr)
{
    /* Set message type */
    switch(hdr->request.opcode) {
    case PROTOCOL_BINARY_CMD_GETQ:
    case PROTOCOL_BINARY_CMD_GET:
    case PROTOCOL_BINARY_CMD_GETKQ:
    case PROTOCOL_BINARY_CMD_GETK:
        if (hdr->request.cas != 0) {
            r->type = MSG_REQ_MC_GETS;
        } else {
            r->type = MSG_REQ_MC_GET;
        }
        break;

    case PROTOCOL_BINARY_CMD_GATQ:
    case PROTOCOL_BINARY_CMD_GAT:
    case PROTOCOL_BINARY_CMD_GATKQ:
    case PROTOCOL_BINARY_CMD_GATK:
        r->type = MSG_REQ_MC_GET_AND_TOUCH;
        break;

    case PROTOCOL_BINARY_CMD_SET:
    case PROTOCOL_BINARY_CMD_SETQ:
        r->type = MSG_REQ_MC_SET;
        break;

    case PROTOCOL_BINARY_CMD_ADD:
    case PROTOCOL_BINARY_CMD_ADDQ:
        r->type = MSG_REQ_MC_ADD;
        break;

    case PROTOCOL_BINARY_CMD_REPLACE:
    case PROTOCOL_BINARY_CMD_REPLACEQ:
        if (hdr->request.cas != 0) {
            r->type = MSG_REQ_MC_CAS;
        } else {
            r->type = MSG_REQ_MC_REPLACE;
        }
        break;

    case PROTOCOL_BINARY_CMD_DELETE:
    case PROTOCOL_BINARY_CMD_DELETEQ:
        r->type = MSG_REQ_MC_DELETE;
        break;

    case PROTOCOL_BINARY_CMD_INCREMENT:
    case PROTOCOL_BINARY_CMD_INCREMENTQ:
        r->type = MSG_REQ_MC_INCR;
        break;

    case PROTOCOL_BINARY_CMD_DECREMENT:
    case PROTOCOL_BINARY_CMD_DECREMENTQ:
        r->type = MSG_REQ_MC_DECR;
        break;

    case PROTOCOL_BINARY_CMD_QUIT:
    case PROTOCOL_BINARY_CMD_QUITQ:
        r->type = MSG_REQ_MC_QUIT;
        r->quit = 1;
        break;

    case PROTOCOL_BINARY_CMD_NOOP:
        r->type = MSG_REQ_MC_NOOP;
        break;

    case PROTOCOL_BINARY_CMD_APPEND:
    case PROTOCOL_BINARY_CMD_APPENDQ:
        r->type = MSG_REQ_MC_APPEND;
        break;

    case PROTOCOL_BINARY_CMD_PREPEND:
    case PROTOCOL_BINARY_CMD_PREPENDQ:
        r->type = MSG_REQ_MC_PREPEND;
        break;

    default:
        r->type = MSG_UNKNOWN;
        break;
    }
    /* Set noreply flag when applicable */
    switch (hdr->request.opcode) {
    case PROTOCOL_BINARY_CMD_SETQ:
    case PROTOCOL_BINARY_CMD_ADDQ:
    case PROTOCOL_BINARY_CMD_REPLACEQ:
    case PROTOCOL_BINARY_CMD_DELETEQ:
    case PROTOCOL_BINARY_CMD_INCREMENTQ:
    case PROTOCOL_BINARY_CMD_DECREMENTQ:
    case PROTOCOL_BINARY_CMD_APPENDQ:
    case PROTOCOL_BINARY_CMD_PREPENDQ:
        r->noreply = 1;
        break;

    case PROTOCOL_BINARY_CMD_GETQ:
    case PROTOCOL_BINARY_CMD_GETKQ:
    case PROTOCOL_BINARY_CMD_GATQ:
    case PROTOCOL_BINARY_CMD_GATKQ:
        r->bufferable = 1;
        break;

    case PROTOCOL_BINARY_CMD_NOOP:
        r->broadcastable = 1;
        break;

    default:
        break;
    }
    /* Alter on the fly the silent get command to ensure replies. */
    switch (hdr->request.opcode) {
    case PROTOCOL_BINARY_CMD_GETQ:
        hdr->request.opcode = PROTOCOL_BINARY_CMD_GET;
        break;
    case PROTOCOL_BINARY_CMD_GETKQ:
        hdr->request.opcode = PROTOCOL_BINARY_CMD_GETK;
        break;
    case PROTOCOL_BINARY_CMD_GATQ:
        hdr->request.opcode = PROTOCOL_BINARY_CMD_GAT;
        break;
    case PROTOCOL_BINARY_CMD_GATKQ:
        hdr->request.opcode = PROTOCOL_BINARY_CMD_GATK;
        break;

    default:
        break;
    }
}

void
mcdbin_parse_req(struct msg *r)
{
    mcdbin_parse_packet(r, PROTOCOL_BINARY_REQ, mcdbin_parse_req_header,
                        mcdbin_get_req_lenvalues, true);
}

static void
mcdbin_get_rsp_lenvalues(protocol_binary_header *hdr, uint8_t *extlen,
                         uint16_t *keylen, uint32_t *valuelen)
{
    if (extlen != NULL) {
        *extlen = hdr->response.extlen;
    }
    if (keylen != NULL) {
        *keylen = ntohs(hdr->response.keylen);
    }
    if (valuelen != NULL) {
        *valuelen = ntohl(hdr->response.bodylen) - ntohs(hdr->response.keylen) -
                    hdr->response.extlen;
    }
}

static void
mcdbin_parse_rsp_header(struct msg *r, protocol_binary_header *hdr)
{
    uint16_t status;
    /*msg_dump(r);*/
    status = ntohs(hdr->response.status);
    switch (status) {
    case PROTOCOL_BINARY_RESPONSE_SUCCESS:
        switch(hdr->response.opcode) {
        case PROTOCOL_BINARY_CMD_GET:
        case PROTOCOL_BINARY_CMD_GETQ:
        case PROTOCOL_BINARY_CMD_GETK:
        case PROTOCOL_BINARY_CMD_GETKQ:
        case PROTOCOL_BINARY_CMD_GAT:
        case PROTOCOL_BINARY_CMD_GATQ:
        case PROTOCOL_BINARY_CMD_GATK:
        case PROTOCOL_BINARY_CMD_GATKQ:
            r->type = MSG_RSP_MC_VALUE;
            break;
        case PROTOCOL_BINARY_CMD_SET:
        case PROTOCOL_BINARY_CMD_SETQ:
        case PROTOCOL_BINARY_CMD_ADD:
        case PROTOCOL_BINARY_CMD_ADDQ:
        case PROTOCOL_BINARY_CMD_REPLACE:
        case PROTOCOL_BINARY_CMD_REPLACEQ:
        case PROTOCOL_BINARY_CMD_APPEND:
        case PROTOCOL_BINARY_CMD_APPENDQ:
        case PROTOCOL_BINARY_CMD_PREPEND:
        case PROTOCOL_BINARY_CMD_PREPENDQ:
            r->type = MSG_RSP_MC_STORED;
            break;
        case PROTOCOL_BINARY_CMD_DELETE:
        case PROTOCOL_BINARY_CMD_DELETEQ:
            r->type = MSG_RSP_MC_DELETED;
            break;
        case PROTOCOL_BINARY_CMD_INCREMENT:
        case PROTOCOL_BINARY_CMD_INCREMENTQ:
        case PROTOCOL_BINARY_CMD_DECREMENT:
        case PROTOCOL_BINARY_CMD_DECREMENTQ:
            r->type = MSG_RSP_MC_NUM;
            break;
        default:
            r->type = MSG_RSP_MC_NOOP;
        }
        break;
    case PROTOCOL_BINARY_RESPONSE_KEY_ENOENT:
        switch(hdr->response.opcode) {
        case PROTOCOL_BINARY_CMD_GET:
        case PROTOCOL_BINARY_CMD_GETQ:
        case PROTOCOL_BINARY_CMD_GETK:
        case PROTOCOL_BINARY_CMD_GETKQ:
        case PROTOCOL_BINARY_CMD_GAT:
        case PROTOCOL_BINARY_CMD_GATQ:
        case PROTOCOL_BINARY_CMD_GATK:
        case PROTOCOL_BINARY_CMD_GATKQ:
            r->type = MSG_RSP_MC_END;
            break;
        case PROTOCOL_BINARY_CMD_REPLACE:
        case PROTOCOL_BINARY_CMD_REPLACEQ:
            r->type = MSG_RSP_MC_NOT_STORED;
            break;
        default:
            r->type = MSG_RSP_MC_NOT_FOUND;
        }
        break;
    case PROTOCOL_BINARY_RESPONSE_KEY_EEXISTS:
        switch(hdr->response.opcode) {
        case PROTOCOL_BINARY_CMD_ADD:
        case PROTOCOL_BINARY_CMD_ADDQ:
            r->type = MSG_RSP_MC_NOT_STORED;
            break;
        default:
            r->type = MSG_RSP_MC_EXISTS;
        }
        break;
    case PROTOCOL_BINARY_RESPONSE_NOT_STORED:
        r->type = MSG_RSP_MC_NOT_STORED;
        break;
    case PROTOCOL_BINARY_RESPONSE_E2BIG:
    case PROTOCOL_BINARY_RESPONSE_ENOMEM:
        r->type = MSG_RSP_MC_SERVER_ERROR;
        break;
    case PROTOCOL_BINARY_RESPONSE_DELTA_BADVAL:
        r->type = MSG_RSP_MC_CLIENT_ERROR;
        break;
    default:
        r->type = MSG_RSP_MC_ERROR;
    }
}

void
mcdbin_parse_rsp(struct msg *r)
{
    mcdbin_parse_packet(r, PROTOCOL_BINARY_RES, mcdbin_parse_rsp_header,
                        mcdbin_get_rsp_lenvalues, false);
}

void
mcdbin_pre_splitcopy(struct mbuf *mbuf, void *arg)
{
}

rstatus_t
mcdbin_post_splitcopy(struct msg *r)
{
    return NC_OK;
}

void
mcdbin_pre_coalesce(struct msg *r)
{
    struct msg *pr = r->peer; /* peer request */
    struct mbuf *mbuf;
    protocol_binary_header *hdr;

    ASSERT(!r->request);
    ASSERT(pr->request);

    if (pr->frag_id == 0) {
        /* do nothing if not a response to a fragmented request */
        return;
    }

    switch(r->type) {
    case MSG_RSP_MC_NOOP:
        break;
    case MSG_RSP_MC_VALUE:
        if (pr->bufferable) {
            mbuf = STAILQ_FIRST(&r->mhdr);
            hdr = (void *)mbuf->pos;
            switch(hdr->response.opcode) {
            case PROTOCOL_BINARY_CMD_GET:
                hdr->response.opcode = PROTOCOL_BINARY_CMD_GETQ;
                break;
            case PROTOCOL_BINARY_CMD_GETK:
                hdr->response.opcode = PROTOCOL_BINARY_CMD_GETKQ;
                break;
            case PROTOCOL_BINARY_CMD_GAT:
                hdr->response.opcode = PROTOCOL_BINARY_CMD_GATQ;
                break;
            case PROTOCOL_BINARY_CMD_GATK:
                hdr->response.opcode = PROTOCOL_BINARY_CMD_GATKQ;
                break;
            default:
                break;
            }
        }
        break;

    case MSG_RSP_MC_END:
        if (pr->bufferable) {
            mbuf = STAILQ_FIRST(&r->mhdr);
            mbuf_rewind(mbuf);
        }
        break;

    default:
        /*
         * Valid responses for a fragmented request are MSG_RSP_MC_VALUE,
         * MSG_RSP_MC_NOOP and MSG_RSP_MC_END. For an invalid response, we
         * send out SERVER_ERROR with EINVAL errno
         */
        mbuf = STAILQ_FIRST(&r->mhdr);
        log_hexdump(LOG_ERR, mbuf->pos, mbuf_length(mbuf), "rsp fragment "
                    "with unknown type %d", r->type);
        pr->error = 1;
        pr->err = EINVAL;
        break;
    }
}

void
mcdbin_post_coalesce(struct msg *r)
{
}

struct msg *
mcdbin_get_terminator(struct msg *r)
{
    protocol_binary_header hdr;
    struct mbuf *mbuf;
    int n;

    hdr.request.magic = PROTOCOL_BINARY_REQ;
    hdr.request.opcode = PROTOCOL_BINARY_CMD_NOOP;
    hdr.request.keylen = htons(0);
    hdr.request.extlen = 0;
    hdr.request.datatype = 0x00; /* RAW DATA */
    hdr.request.vbucket = htons(0);
    hdr.request.bodylen = htonl(0);
    hdr.request.opaque = htonl(0);
    hdr.request.cas = 0;

    r->type = MSG_REQ_MC_NOOP;

    mbuf = mbuf_get();
    if (mbuf == NULL) {
        return NULL;
    }
    mbuf_insert(&r->mhdr, mbuf);

    nc_memcpy(mbuf->last, hdr.bytes, sizeof(hdr.bytes));
    mbuf->last += sizeof(hdr.bytes);
    r->mlen = (uint32_t)sizeof(hdr.bytes);

    return r;
}

struct msg *
mcdbin_generate_error(struct msg *r, err_t err)
{
    protocol_binary_header hdr;
    struct mbuf *mbuf;
    int n;
    size_t len;
    char *errstr = err ? strerror(err) : "unknown";

    len = strlen(errstr);

    hdr.response.magic = PROTOCOL_BINARY_RES;
    hdr.response.opcode = 0;
    hdr.response.keylen = htons(0);
    hdr.response.extlen = 0;
    hdr.response.datatype = 0x00; /* RAW DATA */
    hdr.response.status = htons(err);
    hdr.response.bodylen = htonl(len);
    hdr.response.opaque = htonl(0);
    hdr.response.cas = 0;

    r->type = MSG_RSP_MC_SERVER_ERROR;

    mbuf = mbuf_get();
    if (mbuf == NULL) {
        return NULL;
    }
    mbuf_insert(&r->mhdr, mbuf);

    n = nc_scnprintf(mbuf->last, mbuf_size(mbuf), "%.*s%s",
                     sizeof(hdr.bytes), hdr.bytes, errstr);
    mbuf->last += n;
    r->mlen = (uint32_t)n;

    return r;
}
