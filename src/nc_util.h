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

#ifndef _NC_UTIL_H_
#define _NC_UTIL_H_

#include <stdarg.h>

#define LF                  (uint8_t) 10
#define CR                  (uint8_t) 13
#define CRLF                "\x0d\x0a"
#define CRLF_LEN            (sizeof("\x0d\x0a") - 1)

#define NELEMS(a)           ((sizeof(a)) / sizeof((a)[0]))

#define MIN(a, b)           ((a) < (b) ? (a) : (b))
#define MAX(a, b)           ((a) > (b) ? (a) : (b))

#define SQUARE(d)           ((d) * (d))
#define VAR(s, s2, n)       (((n) < 2) ? 0.0 : ((s2) - SQUARE(s)/(n)) / ((n) - 1))
#define STDDEV(s, s2, n)    (((n) < 2) ? 0.0 : sqrt(VAR((s), (s2), (n))))

#define NC_INET4_ADDRSTRLEN (sizeof("255.255.255.255") - 1)
#define NC_INET6_ADDRSTRLEN \
    (sizeof("ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255") - 1)
#define NC_INET_ADDRSTRLEN  MAX(NC_INET4_ADDRSTRLEN, NC_INET6_ADDRSTRLEN)
#define NC_UNIX_ADDRSTRLEN  \
    (sizeof(struct sockaddr_un) - offsetof(struct sockaddr_un, sun_path))

#define NC_MAXHOSTNAMELEN   256

/*
 * Length of 1 byte, 2 bytes, 4 bytes, 8 bytes and largest integral
 * type (uintmax_t) in ascii, including the null terminator '\0'
 *
 * From stdint.h, we have:
 * # define UINT8_MAX	(255)
 * # define UINT16_MAX	(65535)
 * # define UINT32_MAX	(4294967295U)
 * # define UINT64_MAX	(__UINT64_C(18446744073709551615))
 */
#define NC_UINT8_MAXLEN     (3 + 1)
#define NC_UINT16_MAXLEN    (5 + 1)
#define NC_UINT32_MAXLEN    (10 + 1)
#define NC_UINT64_MAXLEN    (20 + 1)
#define NC_UINTMAX_MAXLEN   NC_UINT64_MAXLEN

/*
 * Make data 'd' or pointer 'p', n-byte aligned, where n is a power of 2
 * of 2.
 */
#define NC_ALIGNMENT        sizeof(unsigned long) /* platform word */
#define NC_ALIGN(d, n)      (((d) + (n - 1)) & ~(n - 1))
#define NC_ALIGN_PTR(p, n)  \
    (void *) (((uintptr_t) (p) + ((uintptr_t) n - 1)) & ~((uintptr_t) n - 1))

/*
 * Wrapper to workaround well known, safe, implicit type conversion when
 * invoking system calls.
 */
#define nc_gethostname(_name, _len) \
    gethostname((char *)_name, (size_t)_len)

#define nc_atoi(_line, _n)          \
    _nc_atoi((uint8_t *)_line, (size_t)_n)

int nc_set_blocking(int sd);
int nc_set_nonblocking(int sd);
int nc_set_reuseaddr(int sd);
int nc_set_tcpnodelay(int sd);
int nc_set_linger(int sd, int timeout);
int nc_set_sndbuf(int sd, int size);
int nc_set_rcvbuf(int sd, int size);
int nc_get_soerror(int sd);
int nc_get_sndbuf(int sd);
int nc_get_rcvbuf(int sd);

int _nc_atoi(uint8_t *line, size_t n);
bool nc_valid_port(int n);

/*
 * Memory allocation and free wrappers.
 *
 * These wrappers enables us to loosely detect double free, dangling
 * pointer access and zero-byte alloc.
 */
#define nc_alloc(_s)                    \
    _nc_alloc((size_t)(_s), __FILE__, __LINE__)

#define nc_zalloc(_s)                   \
    _nc_zalloc((size_t)(_s), __FILE__, __LINE__)

#define nc_calloc(_n, _s)               \
    _nc_calloc((size_t)(_n), (size_t)(_s), __FILE__, __LINE__)

#define nc_realloc(_p, _s)              \
    _nc_realloc(_p, (size_t)(_s), __FILE__, __LINE__)

#define nc_free(_p) do {                \
    _nc_free(_p, __FILE__, __LINE__);   \
    (_p) = NULL;                        \
} while (0)

void *_nc_alloc(size_t size, const char *name, int line);
void *_nc_zalloc(size_t size, const char *name, int line);
void *_nc_calloc(size_t nmemb, size_t size, const char *name, int line);
void *_nc_realloc(void *ptr, size_t size, const char *name, int line);
void _nc_free(void *ptr, const char *name, int line);

/*
 * Wrappers to send or receive n byte message on a blocking
 * socket descriptor.
 */
#define nc_sendn(_s, _b, _n)    \
    _nc_sendn(_s, _b, (size_t)(_n))

#define nc_recvn(_s, _b, _n)    \
    _nc_recvn(_s, _b, (size_t)(_n))

/*
 * Wrappers to read or write data to/from (multiple) buffers
 * to a file or socket descriptor.
 */
#define nc_read(_d, _b, _n)     \
    read(_d, _b, (size_t)(_n))

#define nc_readv(_d, _b, _n)    \
    readv(_d, _b, (int)(_n))

#define nc_write(_d, _b, _n)    \
    write(_d, _b, (size_t)(_n))

#define nc_writev(_d, _b, _n)   \
    writev(_d, _b, (int)(_n))

ssize_t _nc_sendn(int sd, const void *vptr, size_t n);
ssize_t _nc_recvn(int sd, void *vptr, size_t n);

/*
 * Wrappers for defining custom assert based on whether macro
 * NC_ASSERT_PANIC or NC_ASSERT_LOG was defined at the moment
 * ASSERT was called.
 */
#ifdef NC_ASSERT_PANIC

#define ASSERT(_x) do {                         \
    if (!(_x)) {                                \
        nc_assert(#_x, __FILE__, __LINE__, 1);  \
    }                                           \
} while (0)

#define NOT_REACHED() ASSERT(0)

#elif NC_ASSERT_LOG

#define ASSERT(_x) do {                         \
    if (!(_x)) {                                \
        nc_assert(#_x, __FILE__, __LINE__, 0);  \
    }                                           \
} while (0)

#define NOT_REACHED() ASSERT(0)

#else

#define ASSERT(_x)

#define NOT_REACHED()

#endif

void nc_assert(const char *cond, const char *file, int line, int panic);
void nc_stacktrace(int skip_count);

int _scnprintf(char *buf, size_t size, const char *fmt, ...);
int _vscnprintf(char *buf, size_t size, const char *fmt, va_list args);
int64_t nc_usec_now(void);
int64_t nc_msec_now(void);

/*
 * Address resolution for internet (ipv4 and ipv6) and unix domain
 * socket address.
 */

struct sockinfo {
    int       family;              /* socket address family */
    socklen_t addrlen;             /* socket address length */
    union {
        struct sockaddr_in  in;    /* ipv4 socket address */
        struct sockaddr_in6 in6;   /* ipv6 socket address */
        struct sockaddr_un  un;    /* unix domain address */
    } addr;
};

int nc_resolve(struct string *name, int port, struct sockinfo *si);
char *nc_unresolve_addr(struct sockaddr *addr, socklen_t addrlen);
char *nc_unresolve_peer_desc(int sd);
char *nc_unresolve_desc(int sd);

#endif
