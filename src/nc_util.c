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
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <nc_core.h>

#ifdef NC_HAVE_BACKTRACE
# include <execinfo.h>
#endif

int
nc_set_blocking(int sd)
{
    int flags;

    flags = fcntl(sd, F_GETFL, 0);
    if (flags < 0) {
        return flags;
    }

    return fcntl(sd, F_SETFL, flags & ~O_NONBLOCK);
}

int
nc_set_nonblocking(int sd)
{
    int flags;

    flags = fcntl(sd, F_GETFL, 0);
    if (flags < 0) {
        return flags;
    }

    return fcntl(sd, F_SETFL, flags | O_NONBLOCK);
}

int
nc_set_reuseaddr(int sd)
{
    int reuse;
    socklen_t len;

    reuse = 1;
    len = sizeof(reuse);

    return setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &reuse, len);
}

/*
 * Disable Nagle algorithm on TCP socket.
 *
 * This option helps to minimize transmit latency by disabling coalescing
 * of data to fill up a TCP segment inside the kernel. Sockets with this
 * option must use readv() or writev() to do data transfer in bulk and
 * hence avoid the overhead of small packets.
 */
int
nc_set_tcpnodelay(int sd)
{
    int nodelay;
    socklen_t len;

    nodelay = 1;
    len = sizeof(nodelay);

    return setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, &nodelay, len);
}

int
nc_set_linger(int sd, int timeout)
{
    struct linger linger;
    socklen_t len;

    linger.l_onoff = 1;
    linger.l_linger = timeout;

    len = sizeof(linger);

    return setsockopt(sd, SOL_SOCKET, SO_LINGER, &linger, len);
}

int
nc_set_tcpkeepalive(int sd)
{
    int val = 1;
    return setsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val));
}

int
nc_set_sndbuf(int sd, int size)
{
    socklen_t len;

    len = sizeof(size);

    return setsockopt(sd, SOL_SOCKET, SO_SNDBUF, &size, len);
}

int
nc_set_rcvbuf(int sd, int size)
{
    socklen_t len;

    len = sizeof(size);

    return setsockopt(sd, SOL_SOCKET, SO_RCVBUF, &size, len);
}

int
nc_get_soerror(int sd)
{
    int status, err;
    socklen_t len;

    err = 0;
    len = sizeof(err);

    status = getsockopt(sd, SOL_SOCKET, SO_ERROR, &err, &len);
    if (status == 0) {
        errno = err;
    }

    return status;
}

int
nc_get_sndbuf(int sd)
{
    int status, size;
    socklen_t len;

    size = 0;
    len = sizeof(size);

    status = getsockopt(sd, SOL_SOCKET, SO_SNDBUF, &size, &len);
    if (status < 0) {
        return status;
    }

    return size;
}

int
nc_get_rcvbuf(int sd)
{
    int status, size;
    socklen_t len;

    size = 0;
    len = sizeof(size);

    status = getsockopt(sd, SOL_SOCKET, SO_RCVBUF, &size, &len);
    if (status < 0) {
        return status;
    }

    return size;
}

int
_nc_atoi(uint8_t *line, size_t n)
{
    int value;

    if (n == 0) {
        return -1;
    }

    for (value = 0; n--; line++) {
        if (*line < '0' || *line > '9') {
            return -1;
        }

        value = value * 10 + (*line - '0');
    }

    if (value < 0) {
        return -1;
    }

    return value;
}

bool
nc_valid_port(int n)
{
    if (n < 1 || n > UINT16_MAX) {
        return false;
    }

    return true;
}

void *
_nc_alloc(size_t size, const char *name, int line)
{
    void *p;

    ASSERT(size != 0);

    p = malloc(size);
    if (p == NULL) {
        log_error("malloc(%zu) failed @ %s:%d", size, name, line);
    } else {
        log_debug(LOG_VVERB, "malloc(%zu) at %p @ %s:%d", size, p, name, line);
    }

    return p;
}

void *
_nc_zalloc(size_t size, const char *name, int line)
{
    void *p;

    p = _nc_alloc(size, name, line);
    if (p != NULL) {
        memset(p, 0, size);
    }

    return p;
}

void *
_nc_calloc(size_t nmemb, size_t size, const char *name, int line)
{
    return _nc_zalloc(nmemb * size, name, line);
}

void *
_nc_realloc(void *ptr, size_t size, const char *name, int line)
{
    void *p;

    ASSERT(size != 0);

    p = realloc(ptr, size);
    if (p == NULL) {
        log_error("realloc(%zu) failed @ %s:%d", size, name, line);
    } else {
        log_debug(LOG_VVERB, "realloc(%zu) at %p @ %s:%d", size, p, name, line);
    }

    return p;
}

void
_nc_free(void *ptr, const char *name, int line)
{
    ASSERT(ptr != NULL);
    log_debug(LOG_VVERB, "free(%p) @ %s:%d", ptr, name, line);
    free(ptr);
}

void
nc_stacktrace(int skip_count)
{
#ifdef NC_HAVE_BACKTRACE
    void *stack[64];
    char **symbols;
    int size, i, j;

    size = backtrace(stack, 64);
    symbols = backtrace_symbols(stack, size);
    if (symbols == NULL) {
        return;
    }

    skip_count++; /* skip the current frame also */

    for (i = skip_count, j = 0; i < size; i++, j++) {
        loga("[%d] %s", j, symbols[i]);
    }

    free(symbols);
#endif
}

void
nc_stacktrace_fd(int fd)
{
#ifdef NC_HAVE_BACKTRACE
    void *stack[64];
    int size;

    size = backtrace(stack, 64);
    backtrace_symbols_fd(stack, size, fd);
#endif
}

void
nc_assert(const char *cond, const char *file, int line, int panic)
{
    log_error("assert '%s' failed @ (%s, %d)", cond, file, line);
    if (panic) {
        nc_stacktrace(1);
        abort();
    }
}

int
_vscnprintf(char *buf, size_t size, const char *fmt, va_list args)
{
    int n;

    n = vsnprintf(buf, size, fmt, args);

    /*
     * The return value is the number of characters which would be written
     * into buf not including the trailing '\0'. If size is == 0 the
     * function returns 0.
     *
     * On error, the function also returns 0. This is to allow idiom such
     * as len += _vscnprintf(...)
     *
     * See: http://lwn.net/Articles/69419/
     */
    if (n <= 0) {
        return 0;
    }

    if (n < (int) size) {
        return n;
    }

    return (int)(size - 1);
}

int
_scnprintf(char *buf, size_t size, const char *fmt, ...)
{
    va_list args;
    int n;

    va_start(args, fmt);
    n = _vscnprintf(buf, size, fmt, args);
    va_end(args);

    return n;
}

/*
 * Send n bytes on a blocking descriptor
 */
ssize_t
_nc_sendn(int sd, const void *vptr, size_t n)
{
    size_t nleft;
    ssize_t	nsend;
    const char *ptr;

    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        nsend = send(sd, ptr, nleft, 0);
        if (nsend < 0) {
            if (errno == EINTR) {
                continue;
            }
            return nsend;
        }
        if (nsend == 0) {
            return -1;
        }

        nleft -= (size_t)nsend;
        ptr += nsend;
    }

    return (ssize_t)n;
}

/*
 * Recv n bytes from a blocking descriptor
 */
ssize_t
_nc_recvn(int sd, void *vptr, size_t n)
{
	size_t nleft;
	ssize_t	nrecv;
	char *ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
        nrecv = recv(sd, ptr, nleft, 0);
        if (nrecv < 0) {
            if (errno == EINTR) {
                continue;
            }
            return nrecv;
        }
        if (nrecv == 0) {
            break;
        }

        nleft -= (size_t)nrecv;
        ptr += nrecv;
    }

    return (ssize_t)(n - nleft);
}

/*
 * Return the current time in microseconds since Epoch
 */
int64_t
nc_usec_now(void)
{
    struct timeval now;
    int64_t usec;
    int status;

    status = gettimeofday(&now, NULL);
    if (status < 0) {
        log_error("gettimeofday failed: %s", strerror(errno));
        return -1;
    }

    usec = (int64_t)now.tv_sec * 1000000LL + (int64_t)now.tv_usec;

    return usec;
}

/*
 * Return the current time in milliseconds since Epoch
 */
int64_t
nc_msec_now(void)
{
    return nc_usec_now() / 1000LL;
}

static int
nc_resolve_inet(struct string *name, int port, struct sockinfo *si)
{
    int status;
    struct addrinfo *ai, *cai; /* head and current addrinfo */
    struct addrinfo hints;
    char *node, service[NC_UINTMAX_MAXLEN];
    bool found;

    ASSERT(nc_valid_port(port));

    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_NUMERICSERV;
    hints.ai_family = AF_UNSPEC;     /* AF_INET or AF_INET6 */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_addrlen = 0;
    hints.ai_addr = NULL;
    hints.ai_canonname = NULL;

    if (name != NULL) {
        node = (char *)name->data;
    } else {
        /*
         * If AI_PASSIVE flag is specified in hints.ai_flags, and node is
         * NULL, then the returned socket addresses will be suitable for
         * bind(2)ing a socket that will accept(2) connections. The returned
         * socket address will contain the wildcard IP address.
         */
        node = NULL;
        hints.ai_flags |= AI_PASSIVE;
    }

    nc_snprintf(service, NC_UINTMAX_MAXLEN, "%d", port);

    /*
     * getaddrinfo() returns zero on success or one of the error codes listed
     * in gai_strerror(3) if an error occurs
     */
    status = getaddrinfo(node, service, &hints, &ai);
    if (status != 0) {
        log_error("address resolution of node '%s' service '%s' failed: %s",
                  node, service, gai_strerror(status));
        return -1;
    }

    /*
     * getaddrinfo() can return a linked list of more than one addrinfo,
     * since we requested for both AF_INET and AF_INET6 addresses and the
     * host itself can be multi-homed. Since we don't care whether we are
     * using ipv4 or ipv6, we just use the first address from this collection
     * in the order in which it was returned.
     *
     * The sorting function used within getaddrinfo() is defined in RFC 3484;
     * the order can be tweaked for a particular system by editing
     * /etc/gai.conf
     */
    for (cai = ai, found = false; cai != NULL; cai = cai->ai_next) {
        si->family = cai->ai_family;
        si->addrlen = cai->ai_addrlen;
        nc_memcpy(&si->addr, cai->ai_addr, si->addrlen);
        found = true;
        break;
    }

    freeaddrinfo(ai);

    return !found ? -1 : 0;
}

static int
nc_resolve_unix(struct string *name, struct sockinfo *si)
{
    struct sockaddr_un *un;

    if (name->len >= NC_UNIX_ADDRSTRLEN) {
        return -1;
    }

    un = &si->addr.un;

    un->sun_family = AF_UNIX;
    nc_memcpy(un->sun_path, name->data, name->len);
    un->sun_path[name->len] = '\0';

    si->family = AF_UNIX;
    si->addrlen = sizeof(*un);
    /* si->addr is an alias of un */

    return 0;
}

/*
 * Resolve a hostname and service by translating it to socket address and
 * return it in si
 *
 * This routine is reentrant
 */
int
nc_resolve(struct string *name, int port, struct sockinfo *si)
{
    if (name != NULL && name->data[0] == '/') {
        return nc_resolve_unix(name, si);
    }

    return nc_resolve_inet(name, port, si);
}

/*
 * Unresolve the socket address by translating it to a character string
 * describing the host and service
 *
 * This routine is not reentrant
 */
char *
nc_unresolve_addr(struct sockaddr *addr, socklen_t addrlen)
{
    static char unresolve[NI_MAXHOST + NI_MAXSERV];
    static char host[NI_MAXHOST], service[NI_MAXSERV];
    int status;

    status = getnameinfo(addr, addrlen, host, sizeof(host),
                         service, sizeof(service),
                         NI_NUMERICHOST | NI_NUMERICSERV);
    if (status < 0) {
        return "unknown";
    }

    nc_snprintf(unresolve, sizeof(unresolve), "%s:%s", host, service);

    return unresolve;
}

/*
 * Unresolve the socket descriptor peer address by translating it to a
 * character string describing the host and service
 *
 * This routine is not reentrant
 */
char *
nc_unresolve_peer_desc(int sd)
{
    static struct sockinfo si;
    struct sockaddr *addr;
    socklen_t addrlen;
    int status;

    memset(&si, 0, sizeof(si));
    addr = (struct sockaddr *)&si.addr;
    addrlen = sizeof(si.addr);

    status = getpeername(sd, addr, &addrlen);
    if (status < 0) {
        return "unknown";
    }

    return nc_unresolve_addr(addr, addrlen);
}

/*
 * Unresolve the socket descriptor address by translating it to a
 * character string describing the host and service
 *
 * This routine is not reentrant
 */
char *
nc_unresolve_desc(int sd)
{
    static struct sockinfo si;
    struct sockaddr *addr;
    socklen_t addrlen;
    int status;

    memset(&si, 0, sizeof(si));
    addr = (struct sockaddr *)&si.addr;
    addrlen = sizeof(si.addr);

    status = getsockname(sd, addr, &addrlen);
    if (status < 0) {
        return "unknown";
    }

    return nc_unresolve_addr(addr, addrlen);
}
