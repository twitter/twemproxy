#include <nc_hashkit.h>
#include <nc_conf.h>
#include <nc_util.h>
#include <proto/nc_proto.h>
#include <stdio.h>

static int failures = 0;
static int successes = 0;

static void expect_same_uint32_t(uint32_t expected, uint32_t actual, const char* message) {
    if (expected != actual) {
        printf("FAIL Expected %u, got %u (%s)\n", (unsigned int) expected, (unsigned int) actual, message);
        failures++;
    } else {
        /* printf("PASS (%s)\n", message); */
        successes++;
    }
}

static void expect_same_ptr(const void *expected, const void *actual, const char* message) {
    if (expected != actual) {
        printf("FAIL Expected %p, got %p (%s)\n", expected, actual, message);
        failures++;
    } else {
        /* printf("PASS (%s)\n", message); */
        successes++;
    }
}

static void test_hash_algorithms(void) {
    /* refer to libmemcached tests/hash_results.h */
    expect_same_uint32_t(2297466611U, hash_one_at_a_time("apple", 5), "should have expected one_at_a_time hash for key \"apple\"");
    expect_same_uint32_t(3195025439U, hash_md5("apple", 5), "should have expected md5 hash for key \"apple\"");
    expect_same_uint32_t(3853726576U, ketama_hash("server1-8", strlen("server1-8"), 0), "should have expected ketama_hash for server1-8 index 0");
    expect_same_uint32_t(2667054752U, ketama_hash("server1-8", strlen("server1-8"), 3), "should have expected ketama_hash for server1-8 index 3");
}

static void test_config_parsing(void) {
    char* conf_file = "../conf/nutcracker.yml";
    struct conf * conf = conf_create(conf_file);
    if (conf == NULL) {
        printf("FAIL could not parse %s (this test should be run within src/ folder)\n", conf_file);
        failures++;
    } else {
        printf("PASS parsed %s\n", conf_file);

        conf_destroy(conf);
        successes++;
    }
}

static void test_redis_parse_req_success_case(char* data, int expected_type) {
    struct conn fake_client = {0};
    struct mbuf *m = mbuf_get();
    const int SW_START = 0;  /* Same as SW_START in redis_parse_req */

    struct msg *req = msg_get(&fake_client, 1, 1);
    req->state = SW_START;
    req->token = NULL;
    const size_t datalen = (int)strlen(data);

    /* Copy data into the message */
    mbuf_copy(m, (uint8_t*)data, datalen);
    /* Insert a single buffer into the message mbuf header */
    STAILQ_INIT(&req->mhdr);
    ASSERT(STAILQ_EMPTY(&req->mhdr));
    mbuf_insert(&req->mhdr, m);
    req->pos = m->start;

    redis_parse_req(req);
    expect_same_ptr(m->last, req->pos, "redis_parse_req: expected req->pos to be m->last");
    expect_same_uint32_t(SW_START, req->state, "redis_parse_req: expected full buffer to be parsed");
    expect_same_uint32_t(expected_type, req->type, "redis_parse_req: expected request type to be parsed");
    expect_same_uint32_t(0, fake_client.err, "redis_parse_req: expected no connection error");

    msg_put(req);
    /* mbuf_put(m); */
}

/* Test support for https://redis.io/topics/protocol */
static void test_redis_parse_req_success(void) {
    /* Redis requests from clients are serialized as arrays before sending them (* is array length, $ is string length) */
    test_redis_parse_req_success_case("*2\r\n$3\r\nGET\r\n$3\r\nkey\r\n", MSG_REQ_REDIS_GET);
    test_redis_parse_req_success_case("*2\r\n$4\r\nMGET\r\n$1\r\nx\r\n", MSG_REQ_REDIS_MGET);
    test_redis_parse_req_success_case("*3\r\n$4\r\nMGET\r\n$1\r\nx\r\n$10\r\nabcdefghij\r\n", MSG_REQ_REDIS_MGET);

    test_redis_parse_req_success_case("*3\r\n$3\r\nSET\r\n$10\r\nkey4567890\r\n$5\r\nVALUE\r\n", MSG_REQ_REDIS_SET);
    test_redis_parse_req_success_case("*2\r\n$4\r\nLLEN\r\n$6\r\nmylist\r\n", MSG_REQ_REDIS_LLEN);  /* LLEN command */
    test_redis_parse_req_success_case("*1\r\n$4\r\nPING\r\n", MSG_REQ_REDIS_PING);
}

static void test_redis_parse_rsp_success_case(char* data) {
    int original_failures = failures;
    struct conn fake_client = {0};
    struct mbuf *m = mbuf_get();
    const int SW_START = 0;  /* Same as SW_START in redis_parse_rsp */

    struct msg *rsp = msg_get(&fake_client, 0, 1);
    rsp->state = SW_START;
    rsp->token = NULL;
    const size_t datalen = (int)strlen(data);

    /* Copy data into the message */
    mbuf_copy(m, (uint8_t*)data, datalen);
    /* Insert a single buffer into the message mbuf header */
    STAILQ_INIT(&rsp->mhdr);
    ASSERT(STAILQ_EMPTY(&rsp->mhdr));
    mbuf_insert(&rsp->mhdr, m);
    rsp->pos = m->start;

    redis_parse_rsp(rsp);
    expect_same_ptr(m->last, rsp->pos, "redis_parse_rsp: expected rsp->pos to be m->last");
    expect_same_uint32_t(SW_START, rsp->state, "redis_parse_rsp: expected full buffer to be parsed");
    expect_same_uint32_t(1, rsp->rnarg ? rsp->rnarg : 1, "expected remaining args to be 0 or 1");

    msg_put(rsp);
    /* mbuf_put(m); */
    if (failures > original_failures) {
        fprintf(stderr, "test_redis_parse_rsp_success_case failed for %s", data);
    }
}

/* Test support for https://redis.io/topics/protocol */
static void test_redis_parse_rsp_success(void) {
    test_redis_parse_rsp_success_case("-CUSTOMERR\r\n");  /* Error message without a space */
    test_redis_parse_rsp_success_case("-Error message\r\n");  /* Error message */
    test_redis_parse_rsp_success_case("+OK\r\n");  /* Error message without a space */

    test_redis_parse_rsp_success_case("$6\r\nfoobar\r\n");  /* bulk string */
    test_redis_parse_rsp_success_case("$0\r\n\r\n");  /* empty bulk string */
    test_redis_parse_rsp_success_case("$-1\r\n");  /* null value */
    test_redis_parse_rsp_success_case("*0\r\n");  /* empty array */
    test_redis_parse_rsp_success_case("*2\r\n$3\r\nfoo\r\n$3\r\nbar\r\n");  /* array with 2 bulk strings */
    test_redis_parse_rsp_success_case("*3\r\n:1\r\n:2\r\n:3\r\n");  /* array with 3 integers */
    test_redis_parse_rsp_success_case("*-1\r\n");  /* null array for BLPOP */
}

static void test_memcache_parse_rsp_success_case(char* data, int expected) {
    struct conn fake_client = {0};
    struct mbuf *m = mbuf_get();
    const int SW_START = 0;  /* Same as SW_START in memcache_parse_rsp */

    struct msg *rsp = msg_get(&fake_client, 0, 0);
    rsp->state = SW_START;
    rsp->token = NULL;
    const size_t datalen = (int)strlen(data);

    /* Copy data into the message */
    mbuf_copy(m, (uint8_t*)data, datalen);
    /* Insert a single buffer into the message mbuf header */
    STAILQ_INIT(&rsp->mhdr);
    ASSERT(STAILQ_EMPTY(&rsp->mhdr));
    mbuf_insert(&rsp->mhdr, m);
    rsp->pos = m->start;

    memcache_parse_rsp(rsp);
    expect_same_ptr(m->last, rsp->pos, "memcache_parse_rsp: expected rsp->pos to be m->last");
    expect_same_uint32_t(SW_START, rsp->state, "memcache_parse_rsp: expected state to be SW_START after parsing full buffer");
    expect_same_uint32_t(expected, rsp->type, "memcache_parse_rsp: expected response type to be parsed");
    expect_same_uint32_t(0, fake_client.err, "redis_parse_req: expected no connection error");

    msg_put(rsp);
    /* mbuf_put(m); */
}

static void test_memcache_parse_rsp_success(void) {
    test_memcache_parse_rsp_success_case("END\r\n", MSG_RSP_MC_END);
    test_memcache_parse_rsp_success_case("DELETED\r\n", MSG_RSP_MC_DELETED);
    test_memcache_parse_rsp_success_case("TOUCHED\r\n", MSG_RSP_MC_TOUCHED);
    test_memcache_parse_rsp_success_case("STORED\r\n", MSG_RSP_MC_STORED);
    test_memcache_parse_rsp_success_case("EXISTS\r\n", MSG_RSP_MC_EXISTS);
    test_memcache_parse_rsp_success_case("ERROR\r\n", MSG_RSP_MC_ERROR);
    test_memcache_parse_rsp_success_case("NOT_FOUND\r\n", MSG_RSP_MC_NOT_FOUND);
    test_memcache_parse_rsp_success_case("VALUE key 0 2\r\nab\r\nEND\r\n", MSG_RSP_MC_END);
    test_memcache_parse_rsp_success_case("VALUE key 0 2\r\nab\r\nVALUE key2 0 2\r\ncd\r\nEND\r\n", MSG_RSP_MC_END);
}

static void test_memcache_parse_req_success_case(char* data, int expected) {
    struct conn fake_client = {0};
    struct mbuf *m = mbuf_get();
    const int SW_START = 0;  /* Same as SW_START in memcache_parse_req */

    struct msg *req = msg_get(&fake_client, 1, 0);
    req->state = SW_START;
    req->token = NULL;
    const size_t datalen = (int)strlen(data);

    /* Copy data into the message */
    mbuf_copy(m, (uint8_t*)data, datalen);
    /* Insert a single buffer into the message mbuf header */
    STAILQ_INIT(&req->mhdr);
    ASSERT(STAILQ_EMPTY(&req->mhdr));
    mbuf_insert(&req->mhdr, m);
    req->pos = m->start;

    memcache_parse_req(req);
    expect_same_ptr(m->last, req->pos, "memcache_parse_req: expected req->pos to be m->last");
    expect_same_uint32_t(SW_START, req->state, "memcache_parse_req: expected state to be SW_START after parsing full buffer");
    expect_same_uint32_t(expected, req->type, "memcache_parse_req: expected response type to be parsed");
    expect_same_uint32_t(0, fake_client.err, "redis_parse_req: expected no connection error");

    msg_put(req);
    /* mbuf_put(m); */
}

static void test_memcache_parse_req_success(void) {
    test_memcache_parse_req_success_case("get key\r\n", MSG_REQ_MC_GET);
    test_memcache_parse_req_success_case("gets u\r\n", MSG_REQ_MC_GETS);
    test_memcache_parse_req_success_case("get a b xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\r\n", MSG_REQ_MC_GET);
    test_memcache_parse_req_success_case("set key 0 600 5\r\nvalue\r\n", MSG_REQ_MC_SET);
    test_memcache_parse_req_success_case("set key 0 5 10 noreply\r\nvalue12345\r\n", MSG_REQ_MC_SET);
    test_memcache_parse_req_success_case("add key 0 600 5\r\nvalue\r\n", MSG_REQ_MC_ADD);
    test_memcache_parse_req_success_case("replace key 0 600 5\r\nvalue\r\n", MSG_REQ_MC_REPLACE);
    test_memcache_parse_req_success_case("append key 0 600 5\r\nvalue\r\n", MSG_REQ_MC_APPEND);
    test_memcache_parse_req_success_case("prepend key 0 600 5\r\nvalue\r\n", MSG_REQ_MC_PREPEND);
    test_memcache_parse_req_success_case("cas key 0 600 5 123456\r\nvalue\r\n", MSG_REQ_MC_CAS);
    test_memcache_parse_req_success_case("delete key\r\n", MSG_REQ_MC_DELETE);
    test_memcache_parse_req_success_case("incr key 1\r\n", MSG_REQ_MC_INCR);
    test_memcache_parse_req_success_case("quit\r\n", MSG_REQ_MC_QUIT);
    test_memcache_parse_req_success_case("touch key 12345\r\n", MSG_REQ_MC_TOUCH);
}

int main(int argc, char **argv) {
    struct instance nci = {0};
    nci.mbuf_chunk_size = MBUF_SIZE;
    mbuf_init(&nci);
    msg_init();
    log_init(7, NULL);

    test_hash_algorithms();
    test_config_parsing();
    test_redis_parse_rsp_success();
    test_redis_parse_req_success();
    test_memcache_parse_rsp_success();
    test_memcache_parse_req_success();
    printf("%d successes, %d failures\n", successes, failures);

    msg_deinit();
    mbuf_deinit();
    log_deinit();

    return failures > 0 ? 1 : 0;
}
