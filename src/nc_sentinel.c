#include <nc_core.h>
#include <nc_conf.h>
#include <nc_sentinel.h>

static char *sentinel_reqs[] = {
    INFO_SENTINEL,
    SUB_SWITCH_REDIRECT
};

struct conn *
sentinel_conn(struct server *sentinel)
{
    struct conn *conn;

    /* sentinel has only one connection */
    if (sentinel->ns_conn_q == 0) {
        return conn_get_sentinel(sentinel);
    }
    ASSERT(sentinel->ns_conn_q == 1);

    conn = TAILQ_FIRST(&sentinel->s_conn_q);
    ASSERT(!conn->client && !conn->proxy);
    ASSERT(conn->status == CONN_DISCONNECTED);

    return conn;
}

rstatus_t
sentinel_connect(struct context *ctx, struct server *sentinel)
{
    rstatus_t status;
    struct conn *conn;
    struct msg *msg;
    int cmd_num;
    int i;

    ASSERT(sentinel->sentinel);

    /* get the only connect of sentinel */
    conn = sentinel_conn(sentinel);
    if (conn == NULL) {
        /* can't call sentinel_close, Just set reconnect immediately */
        sentinel->owner->next_sentinel_reconn = 1LL;
        return NC_ENOMEM;
    }

    status = server_connect(ctx, sentinel, conn);
    if(status != NC_OK) {
        sentinel_close(ctx, conn);
        return status;
    }

    /* set keepalive opt on sentinel socket to detect socket dead */
    status = nc_set_keepalive(conn->sd, SENTINEL_KEEPALIVE);
    if (status != NC_OK) {
        log_error("set keepalive on s %d for sentienl server failed: %s",
                  conn->sd, strerror(errno));
        sentinel_close(ctx, conn);
        return status;
    }

    cmd_num = sizeof(sentinel_reqs) / sizeof(char *);
    for (i = 0; i < cmd_num; i++) {
        msg = req_fake(ctx, conn);
        if(msg == NULL) {
            conn->err = errno;
            sentinel_close(ctx, conn);
            return NC_ENOMEM;
        }

        status = msg_append(msg, (uint8_t *)(sentinel_reqs[i]), nc_strlen(sentinel_reqs[i]));
        if (status != NC_OK) {
            conn->err = errno;
            sentinel_close(ctx, conn);
            return status;
        }
    }

    conn->status = CONN_SEND_REQ;

    sentinel->owner->next_sentinel_reconn = 0LL;

    return NC_OK;
}

void
sentinel_close(struct context *ctx, struct conn *conn)
{
    struct server_pool *pool;
    int64_t now;

    pool = ((struct server*)conn->owner)->owner;

    now = nc_usec_now();
    if (now < 0) {
        /* get time failed, we reconnect immediately */
        pool->next_sentinel_reconn = 1LL;
        return;
    }

    pool->next_sentinel_reconn = now + pool->server_retry_timeout;

    conn->status = CONN_DISCONNECTED;

    server_close(ctx, conn);
}

static rstatus_t
sentinel_proc_sentinel_info(struct context *ctx, struct server *sentinel, struct msg *msg)
{
    rstatus_t status;
    int i, master_num, switch_num, server_port;
    struct string server_name, server_ip, sentinel_masters_prefix, tmp_string;
    struct server *server;
    struct mbuf *line_buf;

    string_init(&tmp_string);
    string_init(&server_name);
    string_init(&server_ip);
    string_set_text(&sentinel_masters_prefix, "sentinel_masters");

    line_buf = mbuf_get();
    if (line_buf == NULL) {
        goto error;
    }

    /* get sentinel master num at line 3 */
    msg_read_line(msg, line_buf, 3);
    if (mbuf_length(line_buf) == 0) {
        log_error("read line failed from sentinel ack info when skip line not used.");
        goto error;
    }
    status = mbuf_read_string(line_buf, ':', &tmp_string);
    if (status != NC_OK || string_compare(&sentinel_masters_prefix, &tmp_string)) {
        goto error;
    }
    status = mbuf_read_string(line_buf, CR, &tmp_string);
    if (status != NC_OK) {
        goto error;
    }
    master_num = nc_atoi(tmp_string.data, tmp_string.len);
    if (master_num < 0) {
        log_error("parse master number from sentinel ack info failed.");
        goto error;
    }

    /* skip 3 line in ack info which is not used. */
    msg_read_line(msg, line_buf, 3);
    if (mbuf_length(line_buf) == 0) {
        log_error("read line failed from sentinel ack info when skip line not used.");
        goto error;
    }

    /* parse master info from sentinel ack info */
    switch_num = 0;
    for (i = 0; i < master_num; i++) {
        msg_read_line(msg, line_buf, 1);
        if (mbuf_length(line_buf) == 0) {
            log_error("read line failed from sentinel ack info when parse master item.");
            goto error;
        }

        /* skip master item server name prefix */
        status = mbuf_read_string(line_buf, '=', NULL);
        if (status != NC_OK) {
            log_error("skip server name prefix failed.");
            goto error;
        }

        /* get server name */
        status = mbuf_read_string(line_buf, ',', &server_name);
        if (status != NC_OK) {
            log_error("get server name failed.");
            goto error;
        }

        server = server_find_by_name(ctx, sentinel->owner, &server_name);
        if (server == NULL) {
            log_error("unknown server name:%.*s", server_name.len, server_name.data);
            goto error;
        }

        /* skip master status */
        status = mbuf_read_string(line_buf, ',', NULL);
        if (status != NC_OK) {
            log_error("get master status failed.");
            goto error;
        }

        /* skip ip string prefix name */
        status = mbuf_read_string(line_buf, '=', NULL);
        if (status != NC_OK) {
            log_error("skip master item address prefix failed.");
            goto error;
        }

        /* get server ip string */
        status = mbuf_read_string(line_buf, ':', &server_ip);
        if (status != NC_OK) {
            log_error("get server ip string failed.");
            goto error;
        }

        /* get server port */
        status = mbuf_read_string(line_buf, ',', &tmp_string);
        if (status != NC_OK) {
            log_error("get server port string failed.");
            goto error;
        }
        server_port = nc_atoi(tmp_string.data, tmp_string.len);
        if (server_port < 0) {
            log_error("tanslate server port string to int failed.");
            goto error;
        }

        status = server_switch(ctx, server, &server_ip, server_port);
        /* if server is switched, add switch number */
        if (status == NC_OK) {
            switch_num++;
        }
    }

    if (switch_num > 0) {
        conf_rewrite(ctx);
    }

    status = NC_OK;

done:
    if (line_buf != NULL) {
        mbuf_put(line_buf);
    }
    string_deinit(&tmp_string);
    string_deinit(&server_name);
    string_deinit(&server_ip);
    return status;

error:
    status = NC_ERROR;
    goto done;
}

static rstatus_t
sentinel_proc_acksub(struct context *ctx, struct msg *msg, struct string *sub_channel)
{
    rstatus_t status;
    struct string sub_titile, tmp_string;
    struct mbuf *line_buf;

    string_init(&tmp_string);
    string_set_text(&sub_titile, "subscribe");

    line_buf = mbuf_get();
    if (line_buf == NULL) {
        goto error;
    }

    /* get line in line num 3  for sub titile */
    msg_read_line(msg, line_buf, 3);
    if (mbuf_length(line_buf) == 0) {
        log_error("read line failed from sentinel ack sub when skip line not used.");
        goto error;
    }
    status = mbuf_read_string(line_buf, CR, &tmp_string);
    if (status != NC_OK || string_compare(&sub_titile, &tmp_string)) {
        goto error;
    }

    /* get line in line num 5  for sub channel */
    msg_read_line(msg, line_buf, 2);
    if (mbuf_length(line_buf) == 0) {
        log_error("read line failed from sentinel ack sub when skip line not used.");
        goto error;
    }
    status = mbuf_read_string(line_buf, CR, &tmp_string);
    if (status != NC_OK || string_compare(sub_channel, &tmp_string)) {
        goto error;
    }

    log_debug(LOG_INFO, "success sub channel %.*s from sentinel", sub_channel->len, sub_channel->data);

    status = NC_OK;

done:
    if (line_buf != NULL) {
        mbuf_put(line_buf);
    }
    string_deinit(&tmp_string);
    return status;

error:
    status = NC_ERROR;
    goto done;
}

static rstatus_t
sentinel_proc_pub(struct context *ctx, struct server *sentinel, struct msg *msg)
{
    rstatus_t status;
    int server_port;
    struct string server_name, server_ip, tmp_string, pub_titile;
    struct mbuf *line_buf;
    struct server *server;

    string_init(&tmp_string);
    string_init(&server_name);
    string_init(&server_ip);

    string_set_text(&pub_titile, "message");

    line_buf = mbuf_get();
    if (line_buf == NULL) {
        goto error;
    }

    /* get line in line num 3  for pub titile */
    msg_read_line(msg, line_buf, 3);
    if (mbuf_length(line_buf) == 0) {
        log_error("read line failed from sentinel pmessage when skip line not used.");
        goto error;
    }
    status = mbuf_read_string(line_buf, CR, &tmp_string);
    if (status != NC_OK || string_compare(&pub_titile, &tmp_string)) {
        log_error("pub title error(line info %.*s)", tmp_string.len, tmp_string.data);
        goto error;
    }

    /* get line in line num 7 for pub info */
    msg_read_line(msg, line_buf, 4);
    if (mbuf_length(line_buf) == 0) {
        log_error("read line failed from sentinel pmessage when skip line not used.");
        goto error;
    }

    /* get server */
    status = mbuf_read_string(line_buf, ' ', &server_name);
    if (status != NC_OK) {
        log_error("get server name string failed.");
        goto error;
    }
    server = server_find_by_name(ctx, sentinel->owner, &server_name);
    if (server == NULL) {
        log_error("unknown server name:%.*s", server_name.len, server_name.data);
        goto error;
    }

    /* skip old ip and port string */
    status = mbuf_read_string(line_buf, ' ', NULL);
    if (status != NC_OK) {
        log_error("skip old ip string failed.");
        goto error;
    }
    status = mbuf_read_string(line_buf, ' ', NULL);
    if (status != NC_OK) {
        log_error("skip old port string failed.");
        goto error;
    }

    /* get new server ip string */
    status = mbuf_read_string(line_buf, ' ', &server_ip);
    if (status != NC_OK) {
        log_error("get new server ip string failed.");
        goto error;
    }

    /* get new server port */
    status = mbuf_read_string(line_buf, CR, &tmp_string);
    if (status != NC_OK) {
        log_error("get new server port string failed.");
        goto error;
    }
    server_port = nc_atoi(tmp_string.data, tmp_string.len);
    if (server_port < 0) {
        log_error("tanslate server port string to int failed.");
        goto error;
    }

    status = server_switch(ctx, server, &server_ip, server_port);
    ASSERT(status == NC_OK);
    conf_rewrite(ctx);

    status = NC_OK;

done:
    if (line_buf != NULL) {
        mbuf_put(line_buf);
    }
    string_deinit(&tmp_string);
    string_deinit(&server_ip);
    string_deinit(&server_name);
    return status;

error:
    status = NC_ERROR;
    goto done;
}

void
sentinel_recv_done(struct context *ctx, struct conn *conn, struct msg *msg,
              struct msg *nmsg)
{
    rstatus_t status;
    struct string sub_channel;

    ASSERT(!conn->client && !conn->proxy && conn->sentinel);
    ASSERT(msg != NULL && conn->rmsg == msg);
    ASSERT(!msg->request);
    ASSERT(msg->owner == conn);
    ASSERT(nmsg == NULL || !nmsg->request);
    ASSERT(conn->status != CONN_DISCONNECTED);

    /* enqueue next message (response), if any */
    conn->rmsg = nmsg;

    switch (conn->status) {
    case CONN_SEND_REQ:
        status = sentinel_proc_sentinel_info(ctx, conn->owner, msg);
        if (status == NC_OK) {
            conn->status = CONN_ACK_INFO;
        }
        break;

    case CONN_ACK_INFO:
        string_set_text(&sub_channel, SENTINEL_SWITCH_CHANNEL);
        status = sentinel_proc_acksub(ctx, msg, &sub_channel);
        if (status == NC_OK) {
            conn->status = CONN_ACK_SWITCH_SUB;
        }
        break;

    case CONN_ACK_SWITCH_SUB:
        string_set_text(&sub_channel, SENTINEL_REDIRECT_CHANNEL);
        status = sentinel_proc_acksub(ctx, msg, &sub_channel);
        if (status == NC_OK) {
            conn->status = CONN_ACK_REDIRECT_SUB;
        }
        break;

    case CONN_ACK_REDIRECT_SUB:
        status = sentinel_proc_pub(ctx, conn->owner, msg);
        break;

    default:
        status = NC_ERROR;
    }

    rsp_put(msg);
    
    if (status != NC_OK) {
        log_error("sentinel's response error, close sentinel conn.");
        conn->done = 1;
    }
}
