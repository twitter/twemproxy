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

#ifndef _NC_STRING_H_
#define _NC_STRING_H_

#include <string.h>
#include <sys/types.h>
#include <stdarg.h>

#include <nc_core.h>

struct string {
    uint32_t len;   /* string length */
    uint8_t  *data; /* string data */
};

#define string(_str)   { sizeof(_str) - 1, (uint8_t *)(_str) }
#define null_string    { 0, NULL }

#define string_set_text(_str, _text) do {       \
    (_str)->len = (uint32_t)(sizeof(_text) - 1);\
    (_str)->data = (uint8_t *)(_text);          \
} while (0);

#define string_set_raw(_str, _raw) do {         \
    (_str)->len = (uint32_t)(nc_strlen(_raw));  \
    (_str)->data = (uint8_t *)(_raw);           \
} while (0);

void string_init(struct string *str);
void string_deinit(struct string *str);
bool string_empty(const struct string *str);
rstatus_t string_duplicate(struct string *dst, const struct string *src);
rstatus_t string_copy(struct string *dst, const uint8_t *src, uint32_t srclen);
int string_compare(const struct string *s1, const struct string *s2);

/*
 * Wrapper around common routines for manipulating C character
 * strings
 */
#define nc_memcpy(_d, _c, _n)           \
    memcpy(_d, _c, (size_t)(_n))

#define nc_memmove(_d, _c, _n)          \
    memmove(_d, _c, (size_t)(_n))

#define nc_memchr(_d, _c, _n)           \
    memchr(_d, _c, (size_t)(_n))

#define nc_strlen(_s)                   \
    strlen((char *)(_s))

#define nc_strncmp(_s1, _s2, _n)        \
    strncmp((char *)(_s1), (char *)(_s2), (size_t)(_n))

#define nc_strchr(_p, _l, _c)           \
    _nc_strchr((uint8_t *)(_p), (uint8_t *)(_l), (uint8_t)(_c))

#define nc_strrchr(_p, _s, _c)          \
    _nc_strrchr((uint8_t *)(_p),(uint8_t *)(_s), (uint8_t)(_c))

#define nc_strndup(_s, _n)              \
    (uint8_t *)strndup((char *)(_s), (size_t)(_n));

/*
 * snprintf(s, n, ...) will write at most n - 1 of the characters printed into
 * the output string; the nth character then gets the terminating `\0'; if
 * the return value is greater than or equal to the n argument, the string
 * was too short and some of the printed characters were discarded; the output
 * is always null-terminated.
 *
 * Note that, the return value of snprintf() is always the number of characters
 * that would be printed into the output string, assuming n were limited not
 * including the trailing `\0' used to end output.
 *
 * scnprintf(s, n, ...) is same as snprintf() except, it returns the number
 * of characters printed into the output string not including the trailing '\0'
 */
#define nc_snprintf(_s, _n, ...)        \
    snprintf((char *)(_s), (size_t)(_n), __VA_ARGS__)

#define nc_scnprintf(_s, _n, ...)       \
    _scnprintf((char *)(_s), (size_t)(_n), __VA_ARGS__)

#define nc_vsnprintf(_s, _n, _f, _a)    \
    vsnprintf((char *)(_s), (size_t)(_n), _f, _a)

#define nc_vscnprintf(_s, _n, _f, _a)   \
    _vscnprintf((char *)(_s), (size_t)(_n), _f, _a)

#define nc_strftime(_s, _n, fmt, tm)        \
    (int)strftime((char *)(_s), (size_t)(_n), fmt, tm)

/*
 * A (very) limited version of snprintf
 * @param   to   Destination buffer
 * @param   n    Size of destination buffer
 * @param   fmt  printf() style format string
 * @returns Number of bytes written, including terminating '\0'
 * Supports 'd' 'i' 'u' 'x' 'p' 's' conversion
 * Supports 'l' and 'll' modifiers for integral types
 * Does not support any width/precision
 * Implemented with simplicity, and async-signal-safety in mind
 */
int _safe_vsnprintf(char *to, size_t size, const char *format, va_list ap);
int _safe_snprintf(char *to, size_t n, const char *fmt, ...);

#define nc_safe_snprintf(_s, _n, ...)       \
    _safe_snprintf((char *)(_s), (size_t)(_n), __VA_ARGS__)

#define nc_safe_vsnprintf(_s, _n, _f, _a)   \
    _safe_vsnprintf((char *)(_s), (size_t)(_n), _f, _a)

static inline uint8_t *
_nc_strchr(uint8_t *p, uint8_t *last, uint8_t c)
{
    while (p < last) {
        if (*p == c) {
            return p;
        }
        p++;
    }

    return NULL;
}

static inline uint8_t *
_nc_strrchr(uint8_t *p, uint8_t *start, uint8_t c)
{
    while (p >= start) {
        if (*p == c) {
            return p;
        }
        p--;
    }

    return NULL;
}

#endif
