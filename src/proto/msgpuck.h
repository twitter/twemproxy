#ifndef MSGPUCK_H_INCLUDED
#define MSGPUCK_H_INCLUDED
/*
 * Copyright (c) 2013 MsgPuck Authors
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the
 *    following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * <COPYRIGHT HOLDER> OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/**
 * \file msgpuck.h
 * MsgPuck
 * \brief MsgPuck is a simple and efficient MsgPack encoder/decoder
 * library in a single self-contained file.
 *
 * Usage example:
 * \code
 * // Encode
 * char buf[1024];
 * char *w = buf;
 * w = mp_encode_array(w, 4)
 * w = mp_encode_uint(w, 10);
 * w = mp_encode_str(w, "hello world", strlen("hello world"));
 * w = mp_encode_bool(w, true);
 * w = mp_encode_double(w, 3.1415);
 *
 * // Validate
 * const char *b = buf;
 * int r = mp_check(&b, w);
 * assert(!r)
 * assert(b == w);
 *
 * // Decode
 * uint32_t size;
 * uint64_t ival;
 * const char *sval;
 * uint32_t sval_len;
 * bool bval;
 * double dval;
 *
 * const char *r = buf;
 *
 * size = mp_decode_array(&r);
 * // size is 4
 *
 * ival = mp_decode_uint(&r);
 * // ival is 10;
 *
 * sval = mp_decode_str(&r, &sval_len);
 * // sval is "hello world", sval_len is strlen("hello world")
 *
 * bval = mp_decode_bool(&r);
 * // bval is true
 *
 * dval = mp_decode_double(&r);
 * // dval is 3.1415
 *
 * assert(r == w);
 * \endcode
 *
 * \note Supported compilers.
 * The implementation requires a C99+ or C++03+ compatible compiler.
 *
 * \note Inline functions.
 * The implementation is compatible with both C99 and GNU inline functions.
 * Please define MP_SOURCE 1 before \#include <msgpuck.h> in a single
 * compilation unit. This module will be used to store non-inlined versions of
 * functions and global tables.
 */

#if defined(__cplusplus) && !defined(__STDC_CONSTANT_MACROS)
#define __STDC_CONSTANT_MACROS 1 /* make С++ to be happy */
#endif
#if defined(__cplusplus) && !defined(__STDC_LIMIT_MACROS)
#define __STDC_LIMIT_MACROS 1    /* make С++ to be happy */
#endif
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

#if defined(__cplusplus)
extern "C" {
#endif /* defined(__cplusplus) */

/*
 * {{{ Platform-specific definitions
 */

/** \cond 0 **/

#if defined(__CC_ARM)         /* set the alignment to 1 for armcc compiler */
#define MP_PACKED    __packed
#else
#define MP_PACKED
#endif

#if defined(__GNUC__) && !defined(__GNUC_STDC_INLINE__)
#if !defined(MP_SOURCE)
#define MP_PROTO extern inline
#define MP_IMPL extern inline
#else /* defined(MP_SOURCE) */
#define MP_PROTO
#define MP_IMPL
#endif
#define MP_ALWAYSINLINE
#else /* C99 inline */
#if !defined(MP_SOURCE)
#define MP_PROTO inline
#define MP_IMPL inline
#else /* defined(MP_SOURCE) */
#define MP_PROTO extern inline
#define MP_IMPL inline
#endif
#define MP_ALWAYSINLINE __attribute__((always_inline))
#endif /* GNU inline or C99 inline */

#if !defined __GNUC_MINOR__ || defined __INTEL_COMPILER || \
	defined __SUNPRO_C || defined __SUNPRO_CC
#define MP_GCC_VERSION(major, minor) 0
#else
#define MP_GCC_VERSION(major, minor) (__GNUC__ > (major) || \
	(__GNUC__ == (major) && __GNUC_MINOR__ >= (minor)))
#endif

#if !defined(__has_builtin)
#define __has_builtin(x) 0 /* clang */
#endif

#if MP_GCC_VERSION(2, 9) || __has_builtin(__builtin_expect)
#define mp_likely(x) __builtin_expect((x), 1)
#define mp_unlikely(x) __builtin_expect((x), 0)
#else
#define mp_likely(x) (x)
#define mp_unlikely(x) (x)
#endif

#if MP_GCC_VERSION(4, 5) || __has_builtin(__builtin_unreachable)
#define mp_unreachable() (assert(0), __builtin_unreachable())
#else
MP_PROTO void
mp_unreachable(void) __attribute__((noreturn));
MP_PROTO void
mp_unreachable(void) { assert(0); abort(); }
#define mp_unreachable() (assert(0))
#endif

#define mp_bswap_u8(x) (x) /* just to simplify mp_load/mp_store macroses */

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

#if MP_GCC_VERSION(4, 8) || __has_builtin(__builtin_bswap16)
#define mp_bswap_u16(x) __builtin_bswap16(x)
#else /* !MP_GCC_VERSION(4, 8) */
#define mp_bswap_u16(x) ( \
	(((x) <<  8) & 0xff00) | \
	(((x) >>  8) & 0x00ff) )
#endif

#if MP_GCC_VERSION(4, 3) || __has_builtin(__builtin_bswap32)
#define mp_bswap_u32(x) __builtin_bswap32(x)
#else /* !MP_GCC_VERSION(4, 3) */
#define mp_bswap_u32(x) ( \
	(((x) << 24) & UINT32_C(0xff000000)) | \
	(((x) <<  8) & UINT32_C(0x00ff0000)) | \
	(((x) >>  8) & UINT32_C(0x0000ff00)) | \
	(((x) >> 24) & UINT32_C(0x000000ff)) )
#endif

#if MP_GCC_VERSION(4, 3) || __has_builtin(__builtin_bswap64)
#define mp_bswap_u64(x) __builtin_bswap64(x)
#else /* !MP_GCC_VERSION(4, 3) */
#define mp_bswap_u64(x) (\
	(((x) << 56) & UINT64_C(0xff00000000000000)) | \
	(((x) << 40) & UINT64_C(0x00ff000000000000)) | \
	(((x) << 24) & UINT64_C(0x0000ff0000000000)) | \
	(((x) <<  8) & UINT64_C(0x000000ff00000000)) | \
	(((x) >>  8) & UINT64_C(0x00000000ff000000)) | \
	(((x) >> 24) & UINT64_C(0x0000000000ff0000)) | \
	(((x) >> 40) & UINT64_C(0x000000000000ff00)) | \
	(((x) >> 56) & UINT64_C(0x00000000000000ff)) )
#endif

#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__

#define mp_bswap_u16(x) (x)
#define mp_bswap_u32(x) (x)
#define mp_bswap_u64(x) (x)

#else
#error Unsupported __BYTE_ORDER__
#endif

#if !defined(__FLOAT_WORD_ORDER__)
#define __FLOAT_WORD_ORDER__ __BYTE_ORDER__
#endif /* defined(__FLOAT_WORD_ORDER__) */

#if __FLOAT_WORD_ORDER__ == __ORDER_LITTLE_ENDIAN__
MP_PROTO float
mp_bswap_float(float f);

MP_PROTO double
mp_bswap_double(double d);

MP_IMPL float
mp_bswap_float(float f)
{
	union {
		float f;
		uint32_t n;
	} cast;
	cast.f = f;
	cast.n = mp_bswap_u32(cast.n);
	return cast.f;
}

MP_IMPL double
mp_bswap_double(double d)
{
	union {
		double d;
		uint64_t n;
	} cast;
	cast.d = d;
	cast.n = mp_bswap_u64(cast.n);
	return cast.d;
}
#elif __FLOAT_WORD_ORDER__ == __ORDER_BIG_ENDIAN__
#define mp_bswap_float(x) (x)
#define mp_bswap_double(x) (x)
#else
#error Unsupported __FLOAT_WORD_ORDER__
#endif

#define MP_LOAD_STORE(name, type)						\
MP_PROTO type									\
mp_load_##name(const char **data);						\
MP_IMPL type									\
mp_load_##name(const char **data)						\
{										\
	type val = mp_bswap_##name(*(MP_PACKED type *) *data);			\
	*data += sizeof(type);							\
	return val;								\
}										\
MP_PROTO char *									\
mp_store_##name(char *data, type val);						\
MP_IMPL char *									\
mp_store_##name(char *data, type val)						\
{										\
	*(MP_PACKED type *) (data) = mp_bswap_##name(val);			\
	return data + sizeof(type);						\
}

MP_LOAD_STORE(u8, uint8_t);
MP_LOAD_STORE(u16, uint16_t);
MP_LOAD_STORE(u32, uint32_t);
MP_LOAD_STORE(u64, uint64_t);
MP_LOAD_STORE(float, float);
MP_LOAD_STORE(double, double);

/** \endcond */

/*
 * }}}
 */

/*
 * {{{ API definition
 */

/**
 * \brief MsgPack data types
 */
enum mp_type {
	MP_NIL = 0,
	MP_UINT,
	MP_INT,
	MP_STR,
	MP_BIN,
	MP_ARRAY,
	MP_MAP,
	MP_BOOL,
	MP_FLOAT,
	MP_DOUBLE,
	MP_EXT
};

/**
 * \brief Determine MsgPack type by a first byte \a c of encoded data.
 *
 * Example usage:
 * \code
 * assert(MP_ARRAY == mp_typeof(0x90));
 * \endcode
 *
 * \param c - a first byte of encoded data
 * \return MsgPack type
 */
MP_PROTO __attribute__((pure)) enum mp_type
mp_typeof(const char c);

/**
 * \brief Calculate exact buffer size needed to store an array header of
 * \a size elements. Maximum return value is 5. For performance reasons you
 * can preallocate buffer for maximum size without calling the function.
 * \param size - a number of elements
 * \return buffer size in bytes (max is 5)
 */
MP_PROTO __attribute__((const)) uint32_t
mp_sizeof_array(uint32_t size);

/**
 * \brief Encode an array header of \a size elements.
 *
 * All array members must be encoded after the header.
 *
 * Example usage:
 * \code
 * // Encode
 * char buf[1024];
 * char *w = buf;
 * w = mp_encode_array(w, 2)
 * w = mp_encode_uint(w, 10);
 * w = mp_encode_uint(w, 15);
 *
 * // Decode
 * const char *r = buf;
 * uint32_t size = mp_decode_array(&r);
 * for (uint32_t i = 0; i < size; i++) {
 *     uint64_t val = mp_decode_uint(&r);
 * }
 * assert (r == w);
 * \endcode
 * It is your responsibility to ensure that \a data has enough space.
 * \param data - a buffer
 * \param size - a number of elements
 * \return \a data + \link mp_sizeof_array() mp_sizeof_array(size) \endlink
 * \sa mp_sizeof_array
 */
MP_PROTO char *
mp_encode_array(char *data, uint32_t size);

/**
 * \brief Check that \a cur buffer has enough bytes to decode an array header
 * \param cur buffer
 * \param end end of the buffer
 * \retval 0 - buffer has enough bytes
 * \retval > 0 - the number of remaining bytes to read
 * \pre cur < end
 * \pre mp_typeof(*cur) == MP_ARRAY
 */
MP_PROTO __attribute__((pure)) ptrdiff_t
mp_check_array(const char *cur, const char *end);

/**
 * \brief Decode an array header from MsgPack \a data.
 *
 * All array members must be decoded after the header.
 * \param data - the pointer to a buffer
 * \return the number of elements in an array
 * \post *data = *data + mp_sizeof_array(retval)
 * \sa \link mp_encode_array() An usage example \endlink
 */
MP_PROTO uint32_t
mp_decode_array(const char **data);

/**
 * \brief Calculate exact buffer size needed to store a map header of
 * \a size elements. Maximum return value is 5. For performance reasons you
 * can preallocate buffer for maximum size without calling the function.
 * \param size - a number of elements
 * \return buffer size in bytes (max is 5)
 */
MP_PROTO __attribute__((const)) uint32_t
mp_sizeof_map(uint32_t size);

/**
 * \brief Encode a map header of \a size elements.
 *
 * All map key-value pairs must be encoded after the header.
 *
 * Example usage:
 * \code
 * char buf[1024];
 *
 * // Encode
 * char *w = buf;
 * w = mp_encode_map(b, 2);
 * w = mp_encode_str(b, "key1", 4);
 * w = mp_encode_str(b, "value1", 6);
 * w = mp_encode_str(b, "key2", 4);
 * w = mp_encode_str(b, "value2", 6);
 *
 * // Decode
 * const char *r = buf;
 * uint32_t size = mp_decode_map(&r);
 * for (uint32_t i = 0; i < size; i++) {
 *      // Use switch(mp_typeof(**r)) to support more types
 *     uint32_t key_len, val_len;
 *     const char *key = mp_decode_str(&r, key_len);
 *     const char *val = mp_decode_str(&r, val_len);
 * }
 * assert (r == w);
 * \endcode
 * It is your responsibility to ensure that \a data has enough space.
 * \param data - a buffer
 * \param size - a number of key/value pairs
 * \return \a data + \link mp_sizeof_map() mp_sizeof_map(size)\endlink
 * \sa mp_sizeof_map
 */
MP_PROTO char *
mp_encode_map(char *data, uint32_t size);

/**
 * \brief Check that \a cur buffer has enough bytes to decode a map header
 * \param cur buffer
 * \param end end of the buffer
 * \retval 0 - buffer has enough bytes
 * \retval > 0 - the number of remaining bytes to read
 * \pre cur < end
 * \pre mp_typeof(*cur) == MP_MAP
 */
MP_PROTO __attribute__((pure)) ptrdiff_t
mp_check_map(const char *cur, const char *end);

/**
 * \brief Decode a map header from MsgPack \a data.
 *
 * All map key-value pairs must be decoded after the header.
 * \param data - the pointer to a buffer
 * \return the number of key/value pairs in a map
 * \post *data = *data + mp_sizeof_array(retval)
 * \sa \link mp_encode_map() An usage example \endlink
 */
MP_PROTO uint32_t
mp_decode_map(const char **data);

/**
 * \brief Calculate exact buffer size needed to store an integer \a num.
 * Maximum return value is 9. For performance reasons you can preallocate
 * buffer for maximum size without calling the function.
 * Example usage:
 * \code
 * char **data = ...;
 * char *end = *data;
 * my_buffer_ensure(mp_sizeof_uint(x), &end);
 * // my_buffer_ensure(9, &end);
 * mp_encode_uint(buffer, x);
 * \endcode
 * \param num - a number
 * \return buffer size in bytes (max is 9)
 */
MP_PROTO __attribute__((const)) uint32_t
mp_sizeof_uint(uint64_t num);

/**
 * \brief Calculate exact buffer size needed to store an integer \a num.
 * Maximum return value is 9. For performance reasons you can preallocate
 * buffer for maximum size without calling the function.
 * \param num - a number
 * \return buffer size in bytes (max is 9)
 * \pre \a num < 0
 */
MP_PROTO __attribute__((const)) uint32_t
mp_sizeof_int(int64_t num);

/**
 * \brief Encode an unsigned integer \a num.
 * It is your responsibility to ensure that \a data has enough space.
 * \param data - a buffer
 * \param num - a number
 * \return \a data + mp_sizeof_uint(\a num)
 * \sa \link mp_encode_array() An usage example \endlink
 * \sa mp_sizeof_uint()
 */
MP_PROTO char *
mp_encode_uint(char *data, uint64_t num);

/**
 * \brief Encode a signed integer \a num.
 * It is your responsibility to ensure that \a data has enough space.
 * \param data - a buffer
 * \param num - a number
 * \return \a data + mp_sizeof_int(\a num)
 * \sa \link mp_encode_array() An usage example \endlink
 * \sa mp_sizeof_int()
 * \pre \a num < 0
 */
MP_PROTO char *
mp_encode_int(char *data, int64_t num);

/**
 * \brief Check that \a cur buffer has enough bytes to decode an uint
 * \param cur buffer
 * \param end end of the buffer
 * \retval 0 - buffer has enough bytes
 * \retval > 0 - the number of remaining bytes to read
 * \pre cur < end
 * \pre mp_typeof(*cur) == MP_UINT
 */
MP_PROTO __attribute__((pure)) ptrdiff_t
mp_check_uint(const char *cur, const char *end);

/**
 * \brief Check that \a cur buffer has enough bytes to decode an int
 * \param cur buffer
 * \param end end of the buffer
 * \retval 0 - buffer has enough bytes
 * \retval > 0 - the number of remaining bytes to read
 * \pre cur < end
 * \pre mp_typeof(*cur) == MP_INT
 */
MP_PROTO __attribute__((pure)) ptrdiff_t
mp_check_int(const char *cur, const char *end);

/**
 * \brief Decode an unsigned integer from MsgPack \a data
 * \param data - the pointer to a buffer
 * \return an unsigned number
 * \post *data = *data + mp_sizeof_uint(retval)
 */
MP_PROTO uint64_t
mp_decode_uint(const char **data);

/**
 * \brief Decode a signed integer from MsgPack \a data
 * \param data - the pointer to a buffer
 * \return an unsigned number
 * \post *data = *data + mp_sizeof_int(retval)
 */
MP_PROTO int64_t
mp_decode_int(const char **data);

/**
 * \brief Compare two packed unsigned integers.
 *
 * The function is faster than two mp_decode_uint() calls.
 * \param data_a unsigned int a
 * \param data_b unsigned int b
 * \retval < 0 when \a a < \a b
 * \retval   0 when \a a == \a b
 * \retval > 0 when \a a > \a b
 */
MP_PROTO __attribute__((pure)) int
mp_compare_uint(const char *data_a, const char *data_b);

/**
 * \brief Calculate exact buffer size needed to store a float \a num.
 * The return value is always 5. The function was added to provide integrity of
 * the library.
 * \param num - a float
 * \return buffer size in bytes (always 5)
 */
MP_PROTO __attribute__((const)) uint32_t
mp_sizeof_float(float num);

/**
 * \brief Calculate exact buffer size needed to store a double \a num.
 * The return value is either 5 or 9. The function was added to provide
 * integrity of the library. For performance reasons you can preallocate buffer
 * for maximum size without calling the function.
 * \param num - a double
 * \return buffer size in bytes (5 or 9)
 */
MP_PROTO __attribute__((const)) uint32_t
mp_sizeof_double(double num);

/**
 * \brief Encode a float \a num.
 * It is your responsibility to ensure that \a data has enough space.
 * \param data - a buffer
 * \param num - a float
 * \return \a data + mp_sizeof_float(\a num)
 * \sa mp_sizeof_float()
 * \sa \link mp_encode_array() An usage example \endlink
 */
MP_PROTO char *
mp_encode_float(char *data, float num);

/**
 * \brief Encode a double \a num.
 * It is your responsibility to ensure that \a data has enough space.
 * \param data - a buffer
 * \param num - a float
 * \return \a data + mp_sizeof_double(\a num)
 * \sa \link mp_encode_array() An usage example \endlink
 * \sa mp_sizeof_double()
 */
MP_PROTO char *
mp_encode_double(char *data, double num);

/**
 * \brief Check that \a cur buffer has enough bytes to decode a float
 * \param cur buffer
 * \param end end of the buffer
 * \retval 0 - buffer has enough bytes
 * \retval > 0 - the number of remaining bytes to read
 * \pre cur < end
 * \pre mp_typeof(*cur) == MP_FLOAT
 */
MP_PROTO __attribute__((pure)) ptrdiff_t
mp_check_float(const char *cur, const char *end);

/**
 * \brief Check that \a cur buffer has enough bytes to decode a double
 * \param cur buffer
 * \param end end of the buffer
 * \retval 0 - buffer has enough bytes
 * \retval > 0 - the number of remaining bytes to read
 * \pre cur < end
 * \pre mp_typeof(*cur) == MP_DOUBLE
 */
MP_PROTO __attribute__((pure)) ptrdiff_t
mp_check_double(const char *cur, const char *end);

/**
 * \brief Decode a float from MsgPack \a data
 * \param data - the pointer to a buffer
 * \return a float
 * \post *data = *data + mp_sizeof_float(retval)
 */
MP_PROTO float
mp_decode_float(const char **data);

/**
 * \brief Decode a double from MsgPack \a data
 * \param data - the pointer to a buffer
 * \return a double
 * \post *data = *data + mp_sizeof_double(retval)
 */
MP_PROTO double
mp_decode_double(const char **data);

/**
 * \brief Calculate exact buffer size needed to store a string header of
 * length \a num. Maximum return value is 5. For performance reasons you can
 * preallocate buffer for maximum size without calling the function.
 * \param len - a string length
 * \return size in chars (max is 5)
 */
MP_PROTO __attribute__((const)) uint32_t
mp_sizeof_strl(uint32_t len);

/**
 * \brief Equivalent to mp_sizeof_strl(\a len) + \a len.
 * \param len - a string length
 * \return size in chars (max is 5 + \a len)
 */
MP_PROTO __attribute__((const)) uint32_t
mp_sizeof_str(uint32_t len);

/**
 * \brief Calculate exact buffer size needed to store a binstring header of
 * length \a num. Maximum return value is 5. For performance reasons you can
 * preallocate buffer for maximum size without calling the function.
 * \param len - a string length
 * \return size in chars (max is 5)
 */
MP_PROTO __attribute__((const)) uint32_t
mp_sizeof_binl(uint32_t len);

/**
 * \brief Equivalent to mp_sizeof_binl(\a len) + \a len.
 * \param len - a string length
 * \return size in chars (max is 5 + \a len)
 */
MP_PROTO __attribute__((const)) uint32_t
mp_sizeof_bin(uint32_t len);

/**
 * \brief Encode a string header of length \a len.
 *
 * The function encodes MsgPack header (\em only header) for a string of
 * length \a len. You should append actual string data to the buffer manually
 * after encoding the header (exactly \a len bytes without trailing '\0').
 *
 * This approach is very useful for cases when the total length of the string
 * is known in advance, but the string data is not stored in a single
 * continuous buffer (e.g. network packets).
 *
 * It is your responsibility to ensure that \a data has enough space.
 * Usage example:
 * \code
 * char buffer[1024];
 * char *b = buffer;
 * b = mp_encode_strl(b, hdr.total_len);
 * char *s = b;
 * memcpy(b, pkt1.data, pkt1.len)
 * b += pkt1.len;
 * // get next packet
 * memcpy(b, pkt2.data, pkt2.len)
 * b += pkt2.len;
 * // get next packet
 * memcpy(b, pkt1.data, pkt3.len)
 * b += pkt3.len;
 *
 * // Check that all data was received
 * assert(hdr.total_len == (uint32_t) (b - s))
 * \endcode
 * Hint: you can dynamically reallocate the buffer during the process.
 * \param data - a buffer
 * \param len - a string length
 * \return \a data + mp_sizeof_strl(len)
 * \sa mp_sizeof_strl()
 */
MP_PROTO char *
mp_encode_strl(char *data, uint32_t len);

/**
 * \brief Encode a string of length \a len.
 * The function is equivalent to mp_encode_strl() + memcpy.
 * \param data - a buffer
 * \param str - a pointer to string data
 * \param len - a string length
 * \return \a data + mp_sizeof_str(len) ==
 * data + mp_sizeof_strl(len) + len
 * \sa mp_encode_strl
 */
MP_PROTO char *
mp_encode_str(char *data, const char *str, uint32_t len);

/**
 * \brief Encode a binstring header of length \a len.
 * See mp_encode_strl() for more details.
 * \param data - a bufer
 * \param len - a string length
 * \return data + mp_sizeof_binl(\a len)
 * \sa mp_encode_strl
 */
MP_PROTO char *
mp_encode_binl(char *data, uint32_t len);

/**
 * \brief Encode a binstring of length \a len.
 * The function is equivalent to mp_encode_binl() + memcpy.
 * \param data - a buffer
 * \param str - a pointer to binstring data
 * \param len - a binstring length
 * \return \a data + mp_sizeof_bin(\a len) ==
 * data + mp_sizeof_binl(\a len) + \a len
 * \sa mp_encode_strl
 */
MP_PROTO char *
mp_encode_bin(char *data, const char *str, uint32_t len);

/**
 * \brief Encode a sequence of values according to format string.
 * Example: mp_format(buf, sz, "[%d {%d%s%d%s}]", 42, 0, "false", 1, "true");
 * to get a msgpack array of two items: number 42 and map (0->"false, 2->"true")
 * Does not write items that don't fit to data_size argument.
 *
 * \param data - a buffer
 * \param data_size - a buffer size
 * \param format - zero-end string, containing structure of resulting
 * msgpack and types of next arguments.
 * Format can contain '[' and ']' pairs, defining arrays,
 * '{' and '}' pairs, defining maps, and format specifiers, described below:
 * %d, %i - int
 * %u - unsigned int
 * %ld, %li - long
 * %lu - unsigned long
 * %lld, %lli - long long
 * %llu - unsigned long long
 * %hd, %hi - short
 * %hu - unsigned short
 * %hhd, %hhi - char (as number)
 * %hhu - unsigned char (as number)
 * %f - float
 * %lf - double
 * %b - bool
 * %s - zero-end string
 * %.*s - string with specified length
 * %% is ignored
 * %<smth else> assert and undefined behaviour
 * NIL - a nil value
 * all other symbols are ignored.
 *
 * \return the number of requred bytes.
 * \retval > data_size means that is not enough space
 * and whole msgpack was not encoded.
 */
MP_PROTO size_t
mp_format(char *data, size_t data_size, const char *format, ...);

/**
 * \brief mp_format variation, taking variable argument list
 * Example:
 *  va_list args;
 *  va_start(args, fmt);
 *  mp_vformat(data, data_size, fmt, args);
 *  va_end(args);
 * \sa \link mp_format()
 */
MP_PROTO size_t
mp_vformat(char *data, size_t data_size, const char *format, va_list args);

/**
 * \brief Check that \a cur buffer has enough bytes to decode a string header
 * \param cur buffer
 * \param end end of the buffer
 * \retval 0 - buffer has enough bytes
 * \retval > 0 - the number of remaining bytes to read
 * \pre cur < end
 * \pre mp_typeof(*cur) == MP_STR
 */
MP_PROTO __attribute__((pure)) ptrdiff_t
mp_check_strl(const char *cur, const char *end);

/**
 * \brief Check that \a cur buffer has enough bytes to decode a binstring header
 * \param cur buffer
 * \param end end of the buffer
 * \retval 0 - buffer has enough bytes
 * \retval > 0 - the number of remaining bytes to read
 * \pre cur < end
 * \pre mp_typeof(*cur) == MP_BIN
 */
MP_PROTO __attribute__((pure)) ptrdiff_t
mp_check_binl(const char *cur, const char *end);

/**
 * \brief Decode a length of a string from MsgPack \a data
 * \param data - the pointer to a buffer
 * \return a length of astring
 * \post *data = *data + mp_sizeof_strl(retval)
 * \sa mp_encode_strl
 */
MP_PROTO uint32_t
mp_decode_strl(const char **data);

/**
 * \brief Decode a string from MsgPack \a data
 * \param data - the pointer to a buffer
 * \param len - the pointer to save a string length
 * \return a pointer to a decoded string
 * \post *data = *data + mp_sizeof_str(*len)
 * \sa mp_encode_binl
 */
MP_PROTO const char *
mp_decode_str(const char **data, uint32_t *len);

/**
 * \brief Decode a length of a binstring from MsgPack \a data
 * \param data - the pointer to a buffer
 * \return a length of a binstring
 * \post *data = *data + mp_sizeof_binl(retval)
 * \sa mp_encode_binl
 */
MP_PROTO uint32_t
mp_decode_binl(const char **data);

/**
 * \brief Decode a binstring from MsgPack \a data
 * \param data - the pointer to a buffer
 * \param len - the pointer to save a binstring length
 * \return a pointer to a decoded binstring
 * \post *data = *data + mp_sizeof_str(*len)
 * \sa mp_encode_binl
 */
MP_PROTO const char *
mp_decode_bin(const char **data, uint32_t *len);

/**
 * \brief Calculate exact buffer size needed to store the nil value.
 * The return value is always 1. The function was added to provide integrity of
 * the library.
 * \return buffer size in bytes (always 1)
 */
MP_PROTO __attribute__((const)) uint32_t
mp_sizeof_nil(void);

/**
 * \brief Encode the nil value.
 * It is your responsibility to ensure that \a data has enough space.
 * \param data - a buffer
 * \return \a data + mp_sizeof_nil()
 * \sa \link mp_encode_array() An usage example \endlink
 * \sa mp_sizeof_nil()
 */
MP_PROTO char *
mp_encode_nil(char *data);

/**
 * \brief Check that \a cur buffer has enough bytes to decode nil
 * \param cur buffer
 * \param end end of the buffer
 * \retval 0 - buffer has enough bytes
 * \retval > 0 - the number of remaining bytes to read
 * \pre cur < end
 * \pre mp_typeof(*cur) == MP_NIL
 */
MP_PROTO __attribute__((pure)) ptrdiff_t
mp_check_nil(const char *cur, const char *end);

/**
 * \brief Decode the nil value from MsgPack \a data
 * \param data - the pointer to a buffer
 * \post *data = *data + mp_sizeof_nil()
 */
MP_PROTO void
mp_decode_nil(const char **data);

/**
 * \brief Calculate exact buffer size needed to store a boolean value.
 * The return value is always 1. The function was added to provide integrity of
 * the library.
 * \return buffer size in bytes (always 1)
 */
MP_PROTO __attribute__((const)) uint32_t
mp_sizeof_bool(bool val);

/**
 * \brief Encode a bool value \a val.
 * It is your responsibility to ensure that \a data has enough space.
 * \param data - a buffer
 * \param val - a bool
 * \return \a data + mp_sizeof_bool(val)
 * \sa \link mp_encode_array() An usage example \endlink
 * \sa mp_sizeof_bool()
 */
MP_PROTO char *
mp_encode_bool(char *data, bool val);

/**
 * \brief Check that \a cur buffer has enough bytes to decode a bool value
 * \param cur buffer
 * \param end end of the buffer
 * \retval 0 - buffer has enough bytes
 * \retval > 0 - the number of remaining bytes to read
 * \pre cur < end
 * \pre mp_typeof(*cur) == MP_BOOL
 */
MP_PROTO __attribute__((pure)) ptrdiff_t
mp_check_bool(const char *cur, const char *end);

/**
 * \brief Decode a bool value from MsgPack \a data
 * \param data - the pointer to a buffer
 * \return a decoded bool value
 * \post *data = *data + mp_sizeof_bool(retval)
 */
MP_PROTO bool
mp_decode_bool(const char **data);

/**
 * \brief Skip one element in a packed \a data.
 *
 * The function is faster than mp_typeof + mp_decode_XXX() combination.
 * For arrays and maps the function also skips all members.
 * For strings and binstrings the function also skips the string data.
 *
 * Usage example:
 * \code
 * char buf[1024];
 *
 * char *w = buf;
 * // First MsgPack object
 * w = mp_encode_uint(w, 10);
 *
 * // Second MsgPack object
 * w = mp_encode_array(w, 4);
 *    w = mp_encode_array(w, 2);
 *         // Begin of an inner array
 *         w = mp_encode_str(w, "second inner 1", 14);
 *         w = mp_encode_str(w, "second inner 2", 14);
 *         // End of an inner array
 *    w = mp_encode_str(w, "second", 6);
 *    w = mp_encode_uint(w, 20);
 *    w = mp_encode_bool(w, true);
 *
 * // Third MsgPack object
 * w = mp_encode_str(w, "third", 5);
 * // EOF
 *
 * const char *r = buf;
 *
 * // First MsgPack object
 * assert(mp_typeof(**r) == MP_UINT);
 * mp_next(&r); // skip the first object
 *
 * // Second MsgPack object
 * assert(mp_typeof(**r) == MP_ARRAY);
 * mp_decode_array(&r);
 *     assert(mp_typeof(**r) == MP_ARRAY); // inner array
 *     mp_next(&r); // -->> skip the entire inner array (with all members)
 *     assert(mp_typeof(**r) == MP_STR); // second
 *     mp_next(&r);
 *     assert(mp_typeof(**r) == MP_UINT); // 20
 *     mp_next(&r);
 *     assert(mp_typeof(**r) == MP_BOOL); // true
 *     mp_next(&r);
 *
 * // Third MsgPack object
 * assert(mp_typeof(**r) == MP_STR); // third
 * mp_next(&r);
 *
 * assert(r == w); // EOF
 *
 * \endcode
 * \param data - the pointer to a buffer
 * \post *data = *data + mp_sizeof_TYPE() where TYPE is mp_typeof(**data)
 */
MP_PROTO void
mp_next(const char **data);

/**
 * \brief Equivalent to mp_next() but also validates MsgPack in \a data.
 * \param data - the pointer to a buffer
 * \param end - the end of a buffer
 * \retval 0 when MsgPack in \a data is valid.
 * \retval != 0 when MsgPack in \a data is not valid.
 * \post *data = *data + mp_sizeof_TYPE() where TYPE is mp_typeof(**data)
 * \post *data is not defined if MsgPack is not valid
 * \sa mp_next()
 */
MP_PROTO int
mp_check(const char **data, const char *end);

/*
 * }}}
 */

/*
 * {{{ Implementation
 */

/** \cond 0 */
extern const enum mp_type mp_type_hint[];
extern const int8_t mp_parser_hint[];

MP_IMPL MP_ALWAYSINLINE enum mp_type
mp_typeof(const char c)
{
	return mp_type_hint[(uint8_t) c];
}

MP_IMPL uint32_t
mp_sizeof_array(uint32_t size)
{
	if (size <= 15) {
		return 1;
	} else if (size <= UINT16_MAX) {
		return 1 + sizeof(uint16_t);
	} else {
		return 1 + sizeof(uint32_t);
	}
}

MP_IMPL char *
mp_encode_array(char *data, uint32_t size)
{
	if (size <= 15) {
		return mp_store_u8(data, 0x90 | size);
	} else if (size <= UINT16_MAX) {
		data = mp_store_u8(data, 0xdc);
		return mp_store_u16(data, size);
		return data;
	} else {
		data = mp_store_u8(data, 0xdd);
		return mp_store_u32(data, size);
	}
}

MP_IMPL ptrdiff_t
mp_check_array(const char *cur, const char *end)
{
	assert(cur < end);
	assert(mp_typeof(*cur) == MP_ARRAY);
	uint8_t c = mp_load_u8(&cur);
	if (mp_likely(!(c & 0x40)))
		return cur - end;

	assert(c >= 0xdc && c <= 0xdd); /* must be checked above by mp_typeof */
	uint32_t hsize = 2U << (c & 0x1); /* 0xdc->2, 0xdd->4 */
	return hsize - (end - cur);
}

MP_PROTO uint32_t
mp_decode_array_slowpath(uint8_t c, const char **data);

MP_IMPL uint32_t
mp_decode_array_slowpath(uint8_t c, const char **data)
{
	uint32_t size;
	switch (c & 0x1) {
	case 0xdc & 0x1:
		size = mp_load_u16(data);
		return size;
	case 0xdd & 0x1:
		size = mp_load_u32(data);
		return size;
	default:
		mp_unreachable();
	}
}

MP_IMPL MP_ALWAYSINLINE uint32_t
mp_decode_array(const char **data)
{
	uint8_t c = mp_load_u8(data);

	if (mp_likely(!(c & 0x40)))
		return (c & 0xf);

	return mp_decode_array_slowpath(c, data);
}

MP_IMPL uint32_t
mp_sizeof_map(uint32_t size)
{
	if (size <= 15) {
		return 1;
	} else if (size <= UINT16_MAX) {
		return 1 + sizeof(uint16_t);
	} else {
		return 1 + sizeof(uint32_t);
	}
}

MP_IMPL char *
mp_encode_map(char *data, uint32_t size)
{
	if (size <= 15) {
		return mp_store_u8(data, 0x80 | size);
	} else if (size <= UINT16_MAX) {
		data = mp_store_u8(data, 0xde);
		data = mp_store_u16(data, size);
		return data;
	} else {
		data = mp_store_u8(data, 0xdf);
		data = mp_store_u32(data, size);
		return data;
	}
}

MP_IMPL ptrdiff_t
mp_check_map(const char *cur, const char *end)
{
	assert(cur < end);
	assert(mp_typeof(*cur) == MP_MAP);
	uint8_t c = mp_load_u8(&cur);
	if (mp_likely((c & ~0xfU) == 0x80))
		return cur - end;

	assert(c >= 0xde && c <= 0xdf); /* must be checked above by mp_typeof */
	uint32_t hsize = 2U << (c & 0x1); /* 0xde->2, 0xdf->4 */
	return hsize - (end - cur);
}

MP_IMPL uint32_t
mp_decode_map(const char **data)
{
	uint8_t c = mp_load_u8(data);
	switch (c) {
	case 0x80 ... 0x8f:
		return c & 0xf;
	case 0xde:
		return mp_load_u16(data);
	case 0xdf:
		return mp_load_u32(data);
	default:
		mp_unreachable();
	}
}

MP_IMPL uint32_t
mp_sizeof_uint(uint64_t num)
{
	if (num <= 0x7f) {
		return 1;
	} else if (num <= UINT8_MAX) {
		return 1 + sizeof(uint8_t);
	} else if (num <= UINT16_MAX) {
		return 1 + sizeof(uint16_t);
	} else if (num <= UINT32_MAX) {
		return 1 + sizeof(uint32_t);
	} else {
		return 1 + sizeof(uint64_t);
	}
}

MP_IMPL uint32_t
mp_sizeof_int(int64_t num)
{
	assert(num < 0);
	if (num >= -0x20) {
		return 1;
	} else if (num >= INT8_MIN && num <= INT8_MAX) {
		return 1 + sizeof(int8_t);
	} else if (num >= INT16_MIN && num <= UINT16_MAX) {
		return 1 + sizeof(int16_t);
	} else if (num >= INT32_MIN && num <= UINT32_MAX) {
		return 1 + sizeof(int32_t);
	} else {
		return 1 + sizeof(int64_t);
	}
}

MP_IMPL ptrdiff_t
mp_check_uint(const char *cur, const char *end)
{
	assert(cur < end);
	assert(mp_typeof(*cur) == MP_UINT);
	uint8_t c = mp_load_u8(&cur);
	return mp_parser_hint[c] - (end - cur);
}

MP_IMPL ptrdiff_t
mp_check_int(const char *cur, const char *end)
{
	assert(cur < end);
	assert(mp_typeof(*cur) == MP_INT);
	uint8_t c = mp_load_u8(&cur);
	return mp_parser_hint[c] - (end - cur);
}

MP_IMPL char *
mp_encode_uint(char *data, uint64_t num)
{
	if (num <= 0x7f) {
		return mp_store_u8(data, num);
	} else if (num <= UINT8_MAX) {
		data = mp_store_u8(data, 0xcc);
		return mp_store_u8(data, num);
	} else if (num <= UINT16_MAX) {
		data = mp_store_u8(data, 0xcd);
		return mp_store_u16(data, num);
	} else if (num <= UINT32_MAX) {
		data = mp_store_u8(data, 0xce);
		return mp_store_u32(data, num);
	} else {
		data = mp_store_u8(data, 0xcf);
		return mp_store_u64(data, num);
	}
}

MP_IMPL char *
mp_encode_int(char *data, int64_t num)
{
	assert(num < 0);
	if (num >= -0x20) {
		return mp_store_u8(data, 0xe0 | num);
	} else if (num >= INT8_MIN) {
		data = mp_store_u8(data, 0xd0);
		return mp_store_u8(data, num);
	} else if (num >= INT16_MIN) {
		data = mp_store_u8(data, 0xd1);
		return mp_store_u16(data, num);
	} else if (num >= INT32_MIN) {
		data = mp_store_u8(data, 0xd2);
		return mp_store_u32(data, num);
	} else {
		data = mp_store_u8(data, 0xd3);
		return mp_store_u64(data, num);
	}
}

MP_IMPL uint64_t
mp_decode_uint(const char **data)
{
	uint8_t c = mp_load_u8(data);

	switch (c) {
	case 0x00 ... 0x7f:
		return c;
	case 0xcc:
		return mp_load_u8(data);
	case 0xcd:
		return mp_load_u16(data);
	case 0xce:
		return mp_load_u32(data);
	case 0xcf:
		return mp_load_u64(data);
	default:
		mp_unreachable();
	}
}

MP_IMPL int
mp_compare_uint(const char *data_a, const char *data_b)
{
	uint8_t ca = mp_load_u8(&data_a);
	uint8_t cb = mp_load_u8(&data_b);

	int r = ca - cb;
	if (r != 0)
		return r;

	if (ca <= 0x7f)
		return 0;

	uint64_t a, b;
	switch (ca & 0x3) {
	case 0xcc & 0x3:
		a = mp_load_u8(&data_a);
		b = mp_load_u8(&data_b);
		break;
	case 0xcd & 0x3:
		a = mp_load_u16(&data_a);
		b = mp_load_u16(&data_b);
		break;
	case 0xce & 0x3:
		a = mp_load_u32(&data_a);
		b = mp_load_u32(&data_b);
		break;
	case 0xcf & 0x3:
		a = mp_load_u64(&data_a);
		b = mp_load_u64(&data_b);
		return a < b ? -1 : a > b;
		break;
	default:
		mp_unreachable();
	}

	int64_t v = (a - b);
	return (v > 0) - (v < 0);
}

MP_IMPL int64_t
mp_decode_int(const char **data)
{
	uint8_t c = mp_load_u8(data);
	switch (c) {
	case 0xe0 ... 0xff:
		return (int8_t) (c);
	case 0xd0:
		return (int8_t) mp_load_u8(data);
	case 0xd1:
		return (int16_t) mp_load_u16(data);
	case 0xd2:
		return (int32_t) mp_load_u32(data);
	case 0xd3:
		return (int64_t) mp_load_u64(data);
	default:
		mp_unreachable();
	}
}

MP_IMPL uint32_t
mp_sizeof_float(float num)
{
	(void) num;
	return 1 + sizeof(float);
}

MP_IMPL uint32_t
mp_sizeof_double(double num)
{
	(void) num;
	return 1 + sizeof(double);
}

MP_IMPL ptrdiff_t
mp_check_float(const char *cur, const char *end)
{
	assert(cur < end);
	assert(mp_typeof(*cur) == MP_FLOAT);
	return 1 + sizeof(float) - (end - cur);
}

MP_IMPL ptrdiff_t
mp_check_double(const char *cur, const char *end)
{
	assert(cur < end);
	assert(mp_typeof(*cur) == MP_DOUBLE);
	return 1 + sizeof(double) - (end - cur);
}

MP_IMPL char *
mp_encode_float(char *data, float num)
{
	data = mp_store_u8(data, 0xca);
	return mp_store_float(data, num);
}

MP_IMPL char *
mp_encode_double(char *data, double num)
{
	data = mp_store_u8(data, 0xcb);
	return mp_store_double(data, num);
}

MP_IMPL float
mp_decode_float(const char **data)
{
	uint8_t c = mp_load_u8(data);
	assert(c == 0xca);
	(void) c;
	return mp_load_float(data);
}

MP_IMPL double
mp_decode_double(const char **data)
{
	uint8_t c = mp_load_u8(data);
	assert(c == 0xcb);
	(void) c;
	return mp_load_double(data);
}

MP_IMPL uint32_t
mp_sizeof_strl(uint32_t len)
{
	if (len <= 31) {
		return 1;
	} else if (len <= UINT8_MAX) {
		return 1 + sizeof(uint8_t);
	} else if (len <= UINT16_MAX) {
		return 1 + sizeof(uint16_t);
	} else {
		return 1 + sizeof(uint32_t);
	}
}

MP_IMPL uint32_t
mp_sizeof_str(uint32_t len)
{
	return mp_sizeof_strl(len) + len;
}

MP_IMPL uint32_t
mp_sizeof_binl(uint32_t len)
{
	if (len <= UINT8_MAX) {
		return 1 + sizeof(uint8_t);
	} else if (len <= UINT16_MAX) {
		return 1 + sizeof(uint16_t);
	} else {
		return 1 + sizeof(uint32_t);
	}
}

MP_IMPL uint32_t
mp_sizeof_bin(uint32_t len)
{
	return mp_sizeof_binl(len) + len;
}

MP_IMPL char *
mp_encode_strl(char *data, uint32_t len)
{
	if (len <= 31) {
		return mp_store_u8(data, 0xa0 | (uint8_t) len);
	} else if (len <= UINT8_MAX) {
		data = mp_store_u8(data, 0xd9);
		return mp_store_u8(data, len);
	} else if (len <= UINT16_MAX) {
		data = mp_store_u8(data, 0xda);
		return mp_store_u16(data, len);
	} else {
		data = mp_store_u8(data, 0xdb);
		return mp_store_u32(data, len);
	}
}

MP_IMPL char *
mp_encode_str(char *data, const char *str, uint32_t len)
{
	data = mp_encode_strl(data, len);
	memcpy(data, str, len);
	return data + len;
}

MP_IMPL char *
mp_encode_binl(char *data, uint32_t len)
{
	if (len <= UINT8_MAX) {
		data = mp_store_u8(data, 0xc4);
		return mp_store_u8(data, len);
	} else if (len <= UINT16_MAX) {
		data = mp_store_u8(data, 0xc5);
		return mp_store_u16(data, len);
	} else {
		data = mp_store_u8(data, 0xc6);
		return mp_store_u32(data, len);
	}
}

MP_IMPL char *
mp_encode_bin(char *data, const char *str, uint32_t len)
{
	data = mp_encode_binl(data, len);
	memcpy(data, str, len);
	return data + len;
}

MP_IMPL ptrdiff_t
mp_check_strl(const char *cur, const char *end)
{
	assert(cur < end);
	assert(mp_typeof(*cur) == MP_STR);

	uint8_t c = mp_load_u8(&cur);
	if (mp_likely(c & ~0x1f) == 0xa0)
		return cur - end;

	assert(c >= 0xd9 && c <= 0xdb); /* must be checked above by mp_typeof */
	uint32_t hsize = 1U << (c & 0x3) >> 1; /* 0xd9->1, 0xda->2, 0xdb->4 */
	return hsize - (end - cur);
}

MP_IMPL ptrdiff_t
mp_check_binl(const char *cur, const char *end)
{
	uint8_t c = mp_load_u8(&cur);
	assert(cur < end);
	assert(mp_typeof(c) == MP_BIN);

	assert(c >= 0xc4 && c <= 0xc6); /* must be checked above by mp_typeof */
	uint32_t hsize = 1U << (c & 0x3); /* 0xc4->1, 0xc5->2, 0xc6->4 */
	return hsize - (end - cur);
}

MP_IMPL uint32_t
mp_decode_strl(const char **data)
{
	uint8_t c = mp_load_u8(data);

	switch (c) {
	case 0xa0 ... 0xbf:
		return c & 0x1f;
	case 0xd9:
		return mp_load_u8(data);
	case 0xda:
		return mp_load_u16(data);
	case 0xdb:
		return mp_load_u32(data);
	default:
		mp_unreachable();
	}
}

MP_IMPL const char *
mp_decode_str(const char **data, uint32_t *len)
{
	assert(len != NULL);

	*len = mp_decode_strl(data);
	const char *str = *data;
	*data += *len;
	return str;
}

MP_IMPL uint32_t
mp_decode_binl(const char **data)
{
	uint8_t c = mp_load_u8(data);

	switch (c) {
	case 0xc4:
		return mp_load_u8(data);
	case 0xc5:
		return mp_load_u16(data);
	case 0xc6:
		return mp_load_u32(data);
	default:
		mp_unreachable();
	}
}

MP_IMPL const char *
mp_decode_bin(const char **data, uint32_t *len)
{
	assert(len != NULL);

	*len = mp_decode_binl(data);
	const char *str = *data;
	*data += *len;
	return str;
}

MP_IMPL uint32_t
mp_sizeof_nil()
{
	return 1;
}

MP_IMPL char *
mp_encode_nil(char *data)
{
	return mp_store_u8(data, 0xc0);
}

MP_IMPL ptrdiff_t
mp_check_nil(const char *cur, const char *end)
{
	assert(cur < end);
	assert(mp_typeof(*cur) == MP_NIL);
	return 1 - (end - cur);
}

MP_IMPL void
mp_decode_nil(const char **data)
{
	uint8_t c = mp_load_u8(data);
	assert(c == 0xc0);
	(void) c;
}

MP_IMPL uint32_t
mp_sizeof_bool(bool val)
{
	(void) val;
	return 1;
}

MP_IMPL char *
mp_encode_bool(char *data, bool val)
{
	return mp_store_u8(data, 0xc2 | (val & 1));
}

MP_IMPL ptrdiff_t
mp_check_bool(const char *cur, const char *end)
{
	assert(cur < end);
	assert(mp_typeof(*cur) == MP_BOOL);
	return 1 - (end - cur);
}

MP_IMPL bool
mp_decode_bool(const char **data)
{
	uint8_t c = mp_load_u8(data);
	switch (c) {
	case 0xc3:
		return true;
	case 0xc2:
		return false;
	default:
		mp_unreachable();
	}
}

/** See mp_parser_hint */
enum {
	MP_HINT = -32,
	MP_HINT_STR_8 = MP_HINT,
	MP_HINT_STR_16 = MP_HINT - 1,
	MP_HINT_STR_32 = MP_HINT - 2,
	MP_HINT_ARRAY_16 = MP_HINT - 3,
	MP_HINT_ARRAY_32 = MP_HINT - 4,
	MP_HINT_MAP_16 = MP_HINT - 5,
	MP_HINT_MAP_32 = MP_HINT - 6,
	MP_HINT_EXT_8 = MP_HINT - 7,
	MP_HINT_EXT_16 = MP_HINT - 8,
	MP_HINT_EXT_32 = MP_HINT - 9
};

MP_PROTO void
mp_next_slowpath(const char **data, int k);

MP_IMPL void
mp_next_slowpath(const char **data, int k)
{
	for (; k > 0; k--) {
		uint8_t c = mp_load_u8(data);
		int l = mp_parser_hint[c];
		if (mp_likely(l >= 0)) {
			*data += l;
			continue;
		} else if (mp_likely(l > MP_HINT)) {
			k -= l;
			continue;
		}

		uint32_t len;
		switch (l) {
		case MP_HINT_STR_8:
			/* MP_STR (8) */
			len = mp_load_u8(data);
			*data += len;
			break;
		case MP_HINT_STR_16:
			/* MP_STR (16) */
			len = mp_load_u16(data);
			*data += len;
			break;
		case MP_HINT_STR_32:
			/* MP_STR (32) */
			len = mp_load_u32(data);
			*data += len;
			break;
		case MP_HINT_ARRAY_16:
			/* MP_ARRAY (16) */
			k += mp_load_u16(data);
			break;
		case MP_HINT_ARRAY_32:
			/* MP_ARRAY (32) */
			k += mp_load_u32(data);
			break;
		case MP_HINT_MAP_16:
			/* MP_MAP (16) */
			k += 2 * mp_load_u16(data);
			break;
		case MP_HINT_MAP_32:
			/* MP_MAP (32) */
			k += 2 * mp_load_u32(data);
			break;
		case MP_HINT_EXT_8:
			/* MP_EXT (8) */
			len = mp_load_u8(data);
			mp_load_u8(data);
			*data += len;
			break;
		case MP_HINT_EXT_16:
			/* MP_EXT (16) */
			len = mp_load_u16(data);
			mp_load_u8(data);
			*data += len;
			break;
		case MP_HINT_EXT_32:
			/* MP_EXT (32) */
			len = mp_load_u32(data);
			mp_load_u8(data);
			*data += len;
			break;
		default:
			mp_unreachable();
		}
	}
}

MP_IMPL void
mp_next(const char **data)
{
	int k = 1;
	for (; k > 0; k--) {
		uint8_t c = mp_load_u8(data);
		int l = mp_parser_hint[c];
		if (mp_likely(l >= 0)) {
			*data += l;
			continue;
		} else if (mp_likely(c == 0xd9)){
			/* MP_STR (8) */
			uint8_t len = mp_load_u8(data);
			*data += len;
			continue;
		} else if (l > MP_HINT) {
			k -= l;
			continue;
		} else {
			*data -= sizeof(uint8_t);
			return mp_next_slowpath(data, k);
		}
	}
}

MP_IMPL int
mp_check(const char **data, const char *end)
{
	int k;
	for (k = 1; k > 0; k--) {
		if (mp_unlikely(*data >= end))
			return 1;

		uint8_t c = mp_load_u8(data);
		int l = mp_parser_hint[c];
		if (mp_likely(l >= 0)) {
			*data += l;
			continue;
		} else if (mp_likely(l > MP_HINT)) {
			k -= l;
			continue;
		}

		uint32_t len;
		switch (l) {
		case MP_HINT_STR_8:
			/* MP_STR (8) */
			if (mp_unlikely(*data + sizeof(uint8_t) > end))
				return 1;
			len = mp_load_u8(data);
			*data += len;
			break;
		case MP_HINT_STR_16:
			/* MP_STR (16) */
			if (mp_unlikely(*data + sizeof(uint16_t) > end))
				return 1;
			len = mp_load_u16(data);
			*data += len;
			break;
		case MP_HINT_STR_32:
			/* MP_STR (32) */
			if (mp_unlikely(*data + sizeof(uint32_t) > end))
				return 1;
			len = mp_load_u32(data);
			*data += len;
			break;
		case MP_HINT_ARRAY_16:
			/* MP_ARRAY (16) */
			if (mp_unlikely(*data + sizeof(uint16_t) > end))
				return 1;
			k += mp_load_u16(data);
			break;
		case MP_HINT_ARRAY_32:
			/* MP_ARRAY (32) */
			if (mp_unlikely(*data + sizeof(uint32_t) > end))
				return 1;
			k += mp_load_u32(data);
			break;
		case MP_HINT_MAP_16:
			/* MP_MAP (16) */
			if (mp_unlikely(*data + sizeof(uint16_t) > end))
				return false;
			k += 2 * mp_load_u16(data);
			break;
		case MP_HINT_MAP_32:
			/* MP_MAP (32) */
			if (mp_unlikely(*data + sizeof(uint32_t) > end))
				return 1;
			k += 2 * mp_load_u32(data);
			break;
		case MP_HINT_EXT_8:
			/* MP_EXT (8) */
			if (mp_unlikely(*data + sizeof(uint8_t) + 1 > end))
				return 1;
			len = mp_load_u8(data);
			mp_load_u8(data);
			*data += len;
			break;
		case MP_HINT_EXT_16:
			/* MP_EXT (16) */
			if (mp_unlikely(*data + sizeof(uint16_t) + 1 > end))
				return 1;
			len = mp_load_u16(data);
			mp_load_u8(data);
			*data += len;
			break;
		case MP_HINT_EXT_32:
			/* MP_EXT (32) */
			if (mp_unlikely(*data + sizeof(uint32_t) + 1 > end))
				return 1;
		        len = mp_load_u32(data);
			mp_load_u8(data);
			*data += len;
			break;
		default:
			mp_unreachable();
		}
	}

	if (mp_unlikely(*data > end))
		return 1;

	return 0;
}

MP_IMPL size_t
mp_vformat(char *data, size_t data_size, const char *format, va_list vl)
{
	size_t result = 0;
	const char *f, *e;

	for (f = format; *f; f++) {
		if (f[0] == '[') {
			uint32_t size = 0;
			int level = 1;
			for (e = f + 1; level && *e; e++) {
				if (*e == '[' || *e == '{') {
					if (level == 1)
						size++;
					level++;
				} else if (*e == ']' || *e == '}') {
					level--;
					/* opened '[' must be closed by ']' */
					assert(level || *e == ']');
				} else if (*e == '%') {
					if (e[1] == '%')
						e++;
					else if (level == 1)
						size++;
				} else if (*e == 'N' && e[1] == 'I'
					   && e[2] == 'L' && level == 1) {
					size++;
				}
			}
			/* opened '[' must be closed */
			assert(level == 0);
			result += mp_sizeof_array(size);
			if (result <= data_size)
				data = mp_encode_array(data, size);
		} else if (f[0] == '{') {
			uint32_t count = 0;
			int level = 1;
			for (e = f + 1; level && *e; e++) {
				if (*e == '[' || *e == '{') {
					if (level == 1)
						count++;
					level++;
				} else if (*e == ']' || *e == '}') {
					level--;
					/* opened '{' must be closed by '}' */
					assert(level || *e == '}');
				} else if (*e == '%') {
					if (e[1] == '%')
						e++;
					else if (level == 1)
						count++;
				} else if (*e == 'N' && e[1] == 'I'
					   && e[2] == 'L' && level == 1) {
					count++;
				}
			}
			/* opened '{' must be closed */
			assert(level == 0);
			/* since map is a pair list, count must be even */
			assert(count % 2 == 0);
			uint32_t size = count / 2;
			result += mp_sizeof_map(size);
			if (result <= data_size)
				data = mp_encode_map(data, size);
		} else if (f[0] == '%') {
			f++;
			assert(f[0]);
			int64_t int_value = 0;
			int int_status = 0; /* 1 - signed, 2 - unsigned */

			if (f[0] == 'd' || f[0] == 'i') {
				int_value = va_arg(vl, int);
				int_status = 1;
			} else if (f[0] == 'u') {
				int_value = va_arg(vl, unsigned int);
				int_status = 2;
			} else if (f[0] == 's') {
				const char *str = va_arg(vl, const char *);
				uint32_t len = (uint32_t)strlen(str);
				result += mp_sizeof_str(len);
				if (result <= data_size)
					data = mp_encode_str(data, str, len);
			} else if (f[0] == '.' && f[1] == '*' && f[2] == 's') {
				uint32_t len = va_arg(vl, uint32_t);
				const char *str = va_arg(vl, const char *);
				result += mp_sizeof_str(len);
				if (result <= data_size)
					data = mp_encode_str(data, str, len);
				f += 2;
			} else if(f[0] == 'f') {
				float v = (float)va_arg(vl, double);
				result += mp_sizeof_float(v);
				if (result <= data_size)
					data = mp_encode_float(data, v);
			} else if(f[0] == 'l' && f[1] == 'f') {
				double v = va_arg(vl, double);
				result += mp_sizeof_double(v);
				if (result <= data_size)
					data = mp_encode_double(data, v);
				f++;
			} else if(f[0] == 'b') {
				bool v = (bool)va_arg(vl, int);
				result += mp_sizeof_bool(v);
				if (result <= data_size)
					data = mp_encode_bool(data, v);
			} else if (f[0] == 'l'
				   && (f[1] == 'd' || f[1] == 'i')) {
				int_value = va_arg(vl, long);
				int_status = 1;
				f++;
			} else if (f[0] == 'l' && f[1] == 'u') {
				int_value = va_arg(vl, unsigned long);
				int_status = 2;
				f++;
			} else if (f[0] == 'l' && f[1] == 'l'
				   && (f[2] == 'd' || f[2] == 'i')) {
				int_value = va_arg(vl, long long);
				int_status = 1;
				f += 2;
			} else if (f[0] == 'l' && f[1] == 'l' && f[2] == 'u') {
				int_value = va_arg(vl, unsigned long long);
				int_status = 2;
				f += 2;
			} else if (f[0] == 'h'
				   && (f[1] == 'd' || f[1] == 'i')) {
				int_value = va_arg(vl, int);
				int_status = 1;
				f++;
			} else if (f[0] == 'h' && f[1] == 'u') {
				int_value = va_arg(vl, unsigned int);
				int_status = 2;
				f++;
			} else if (f[0] == 'h' && f[1] == 'h'
				   && (f[2] == 'd' || f[2] == 'i')) {
				int_value = va_arg(vl, int);
				int_status = 1;
				f += 2;
			} else if (f[0] == 'h' && f[1] == 'h' && f[2] == 'u') {
				int_value = va_arg(vl, unsigned int);
				int_status = 2;
				f += 2;
			} else if (f[0] != '%') {
				/* unexpected format specifier */
				assert(false);
			}

			if (int_status == 1 && int_value < 0) {
				result += mp_sizeof_int(int_value);
				if (result <= data_size)
					data = mp_encode_int(data, int_value);
			} else if(int_status) {
				result += mp_sizeof_uint(int_value);
				if (result <= data_size)
					data = mp_encode_uint(data, int_value);
			}
		} else if (f[0] == 'N' && f[1] == 'I' && f[2] == 'L') {
			result += mp_sizeof_nil();
			if (result <= data_size)
				data = mp_encode_nil(data);
			f += 2;
		}
	}
	return result;
}

MP_IMPL size_t
mp_format(char *data, size_t data_size, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	size_t res = mp_vformat(data, data_size, format, args);
	va_end(args);
	return res;
}

/** \endcond */

/*
 * }}}
 */

/*
 * {{{ Implementation: parser tables
 */

/** \cond 0 */

#if defined(MP_SOURCE)

/**
 * This lookup table used by mp_sizeof() to determine enum mp_type by the first
 * byte of MsgPack element.
 */
const enum mp_type mp_type_hint[256]= {
	/* {{{ MP_UINT (fixed) */
	/* 0x00 */ MP_UINT,
	/* 0x01 */ MP_UINT,
	/* 0x02 */ MP_UINT,
	/* 0x03 */ MP_UINT,
	/* 0x04 */ MP_UINT,
	/* 0x05 */ MP_UINT,
	/* 0x06 */ MP_UINT,
	/* 0x07 */ MP_UINT,
	/* 0x08 */ MP_UINT,
	/* 0x09 */ MP_UINT,
	/* 0x0a */ MP_UINT,
	/* 0x0b */ MP_UINT,
	/* 0x0c */ MP_UINT,
	/* 0x0d */ MP_UINT,
	/* 0x0e */ MP_UINT,
	/* 0x0f */ MP_UINT,
	/* 0x10 */ MP_UINT,
	/* 0x11 */ MP_UINT,
	/* 0x12 */ MP_UINT,
	/* 0x13 */ MP_UINT,
	/* 0x14 */ MP_UINT,
	/* 0x15 */ MP_UINT,
	/* 0x16 */ MP_UINT,
	/* 0x17 */ MP_UINT,
	/* 0x18 */ MP_UINT,
	/* 0x19 */ MP_UINT,
	/* 0x1a */ MP_UINT,
	/* 0x1b */ MP_UINT,
	/* 0x1c */ MP_UINT,
	/* 0x1d */ MP_UINT,
	/* 0x1e */ MP_UINT,
	/* 0x1f */ MP_UINT,
	/* 0x20 */ MP_UINT,
	/* 0x21 */ MP_UINT,
	/* 0x22 */ MP_UINT,
	/* 0x23 */ MP_UINT,
	/* 0x24 */ MP_UINT,
	/* 0x25 */ MP_UINT,
	/* 0x26 */ MP_UINT,
	/* 0x27 */ MP_UINT,
	/* 0x28 */ MP_UINT,
	/* 0x29 */ MP_UINT,
	/* 0x2a */ MP_UINT,
	/* 0x2b */ MP_UINT,
	/* 0x2c */ MP_UINT,
	/* 0x2d */ MP_UINT,
	/* 0x2e */ MP_UINT,
	/* 0x2f */ MP_UINT,
	/* 0x30 */ MP_UINT,
	/* 0x31 */ MP_UINT,
	/* 0x32 */ MP_UINT,
	/* 0x33 */ MP_UINT,
	/* 0x34 */ MP_UINT,
	/* 0x35 */ MP_UINT,
	/* 0x36 */ MP_UINT,
	/* 0x37 */ MP_UINT,
	/* 0x38 */ MP_UINT,
	/* 0x39 */ MP_UINT,
	/* 0x3a */ MP_UINT,
	/* 0x3b */ MP_UINT,
	/* 0x3c */ MP_UINT,
	/* 0x3d */ MP_UINT,
	/* 0x3e */ MP_UINT,
	/* 0x3f */ MP_UINT,
	/* 0x40 */ MP_UINT,
	/* 0x41 */ MP_UINT,
	/* 0x42 */ MP_UINT,
	/* 0x43 */ MP_UINT,
	/* 0x44 */ MP_UINT,
	/* 0x45 */ MP_UINT,
	/* 0x46 */ MP_UINT,
	/* 0x47 */ MP_UINT,
	/* 0x48 */ MP_UINT,
	/* 0x49 */ MP_UINT,
	/* 0x4a */ MP_UINT,
	/* 0x4b */ MP_UINT,
	/* 0x4c */ MP_UINT,
	/* 0x4d */ MP_UINT,
	/* 0x4e */ MP_UINT,
	/* 0x4f */ MP_UINT,
	/* 0x50 */ MP_UINT,
	/* 0x51 */ MP_UINT,
	/* 0x52 */ MP_UINT,
	/* 0x53 */ MP_UINT,
	/* 0x54 */ MP_UINT,
	/* 0x55 */ MP_UINT,
	/* 0x56 */ MP_UINT,
	/* 0x57 */ MP_UINT,
	/* 0x58 */ MP_UINT,
	/* 0x59 */ MP_UINT,
	/* 0x5a */ MP_UINT,
	/* 0x5b */ MP_UINT,
	/* 0x5c */ MP_UINT,
	/* 0x5d */ MP_UINT,
	/* 0x5e */ MP_UINT,
	/* 0x5f */ MP_UINT,
	/* 0x60 */ MP_UINT,
	/* 0x61 */ MP_UINT,
	/* 0x62 */ MP_UINT,
	/* 0x63 */ MP_UINT,
	/* 0x64 */ MP_UINT,
	/* 0x65 */ MP_UINT,
	/* 0x66 */ MP_UINT,
	/* 0x67 */ MP_UINT,
	/* 0x68 */ MP_UINT,
	/* 0x69 */ MP_UINT,
	/* 0x6a */ MP_UINT,
	/* 0x6b */ MP_UINT,
	/* 0x6c */ MP_UINT,
	/* 0x6d */ MP_UINT,
	/* 0x6e */ MP_UINT,
	/* 0x6f */ MP_UINT,
	/* 0x70 */ MP_UINT,
	/* 0x71 */ MP_UINT,
	/* 0x72 */ MP_UINT,
	/* 0x73 */ MP_UINT,
	/* 0x74 */ MP_UINT,
	/* 0x75 */ MP_UINT,
	/* 0x76 */ MP_UINT,
	/* 0x77 */ MP_UINT,
	/* 0x78 */ MP_UINT,
	/* 0x79 */ MP_UINT,
	/* 0x7a */ MP_UINT,
	/* 0x7b */ MP_UINT,
	/* 0x7c */ MP_UINT,
	/* 0x7d */ MP_UINT,
	/* 0x7e */ MP_UINT,
	/* 0x7f */ MP_UINT,
	/* }}} */

	/* {{{ MP_MAP (fixed) */
	/* 0x80 */ MP_MAP,
	/* 0x81 */ MP_MAP,
	/* 0x82 */ MP_MAP,
	/* 0x83 */ MP_MAP,
	/* 0x84 */ MP_MAP,
	/* 0x85 */ MP_MAP,
	/* 0x86 */ MP_MAP,
	/* 0x87 */ MP_MAP,
	/* 0x88 */ MP_MAP,
	/* 0x89 */ MP_MAP,
	/* 0x8a */ MP_MAP,
	/* 0x8b */ MP_MAP,
	/* 0x8c */ MP_MAP,
	/* 0x8d */ MP_MAP,
	/* 0x8e */ MP_MAP,
	/* 0x8f */ MP_MAP,
	/* }}} */

	/* {{{ MP_ARRAY (fixed) */
	/* 0x90 */ MP_ARRAY,
	/* 0x91 */ MP_ARRAY,
	/* 0x92 */ MP_ARRAY,
	/* 0x93 */ MP_ARRAY,
	/* 0x94 */ MP_ARRAY,
	/* 0x95 */ MP_ARRAY,
	/* 0x96 */ MP_ARRAY,
	/* 0x97 */ MP_ARRAY,
	/* 0x98 */ MP_ARRAY,
	/* 0x99 */ MP_ARRAY,
	/* 0x9a */ MP_ARRAY,
	/* 0x9b */ MP_ARRAY,
	/* 0x9c */ MP_ARRAY,
	/* 0x9d */ MP_ARRAY,
	/* 0x9e */ MP_ARRAY,
	/* 0x9f */ MP_ARRAY,
	/* }}} */

	/* {{{ MP_STR (fixed) */
	/* 0xa0 */ MP_STR,
	/* 0xa1 */ MP_STR,
	/* 0xa2 */ MP_STR,
	/* 0xa3 */ MP_STR,
	/* 0xa4 */ MP_STR,
	/* 0xa5 */ MP_STR,
	/* 0xa6 */ MP_STR,
	/* 0xa7 */ MP_STR,
	/* 0xa8 */ MP_STR,
	/* 0xa9 */ MP_STR,
	/* 0xaa */ MP_STR,
	/* 0xab */ MP_STR,
	/* 0xac */ MP_STR,
	/* 0xad */ MP_STR,
	/* 0xae */ MP_STR,
	/* 0xaf */ MP_STR,
	/* 0xb0 */ MP_STR,
	/* 0xb1 */ MP_STR,
	/* 0xb2 */ MP_STR,
	/* 0xb3 */ MP_STR,
	/* 0xb4 */ MP_STR,
	/* 0xb5 */ MP_STR,
	/* 0xb6 */ MP_STR,
	/* 0xb7 */ MP_STR,
	/* 0xb8 */ MP_STR,
	/* 0xb9 */ MP_STR,
	/* 0xba */ MP_STR,
	/* 0xbb */ MP_STR,
	/* 0xbc */ MP_STR,
	/* 0xbd */ MP_STR,
	/* 0xbe */ MP_STR,
	/* 0xbf */ MP_STR,
	/* }}} */

	/* {{{ MP_NIL, MP_BOOL */
	/* 0xc0 */ MP_NIL,
	/* 0xc1 */ MP_EXT, /* never used */
	/* 0xc2 */ MP_BOOL,
	/* 0xc3 */ MP_BOOL,
	/* }}} */

	/* {{{ MP_BIN */
	/* 0xc4 */ MP_BIN,   /* MP_BIN(8)  */
	/* 0xc5 */ MP_BIN,   /* MP_BIN(16) */
	/* 0xc6 */ MP_BIN,   /* MP_BIN(32) */
	/* }}} */

	/* {{{ MP_EXT */
	/* 0xc7 */ MP_EXT,
	/* 0xc8 */ MP_EXT,
	/* 0xc9 */ MP_EXT,
	/* }}} */

	/* {{{ MP_FLOAT, MP_DOUBLE */
	/* 0xca */ MP_FLOAT,
	/* 0xcb */ MP_DOUBLE,
	/* }}} */

	/* {{{ MP_UINT */
	/* 0xcc */ MP_UINT,
	/* 0xcd */ MP_UINT,
	/* 0xce */ MP_UINT,
	/* 0xcf */ MP_UINT,
	/* }}} */

	/* {{{ MP_INT */
	/* 0xd0 */ MP_INT,   /* MP_INT (8)  */
	/* 0xd1 */ MP_INT,   /* MP_INT (16) */
	/* 0xd2 */ MP_INT,   /* MP_INT (32) */
	/* 0xd3 */ MP_INT,   /* MP_INT (64) */
	/* }}} */

	/* {{{ MP_EXT */
	/* 0xd4 */ MP_EXT,   /* MP_INT (8)    */
	/* 0xd5 */ MP_EXT,   /* MP_INT (16)   */
	/* 0xd6 */ MP_EXT,   /* MP_INT (32)   */
	/* 0xd7 */ MP_EXT,   /* MP_INT (64)   */
	/* 0xd8 */ MP_EXT,   /* MP_INT (127)  */
	/* }}} */

	/* {{{ MP_STR */
	/* 0xd9 */ MP_STR,   /* MP_STR(8)  */
	/* 0xda */ MP_STR,   /* MP_STR(16) */
	/* 0xdb */ MP_STR,   /* MP_STR(32) */
	/* }}} */

	/* {{{ MP_ARRAY */
	/* 0xdc */ MP_ARRAY, /* MP_ARRAY(16)  */
	/* 0xdd */ MP_ARRAY, /* MP_ARRAY(32)  */
	/* }}} */

	/* {{{ MP_MAP */
	/* 0xde */ MP_MAP,   /* MP_MAP (16) */
	/* 0xdf */ MP_MAP,   /* MP_MAP (32) */
	/* }}} */

	/* {{{ MP_INT */
	/* 0xe0 */ MP_INT,
	/* 0xe1 */ MP_INT,
	/* 0xe2 */ MP_INT,
	/* 0xe3 */ MP_INT,
	/* 0xe4 */ MP_INT,
	/* 0xe5 */ MP_INT,
	/* 0xe6 */ MP_INT,
	/* 0xe7 */ MP_INT,
	/* 0xe8 */ MP_INT,
	/* 0xe9 */ MP_INT,
	/* 0xea */ MP_INT,
	/* 0xeb */ MP_INT,
	/* 0xec */ MP_INT,
	/* 0xed */ MP_INT,
	/* 0xee */ MP_INT,
	/* 0xef */ MP_INT,
	/* 0xf0 */ MP_INT,
	/* 0xf1 */ MP_INT,
	/* 0xf2 */ MP_INT,
	/* 0xf3 */ MP_INT,
	/* 0xf4 */ MP_INT,
	/* 0xf5 */ MP_INT,
	/* 0xf6 */ MP_INT,
	/* 0xf7 */ MP_INT,
	/* 0xf8 */ MP_INT,
	/* 0xf9 */ MP_INT,
	/* 0xfa */ MP_INT,
	/* 0xfb */ MP_INT,
	/* 0xfc */ MP_INT,
	/* 0xfd */ MP_INT,
	/* 0xfe */ MP_INT,
	/* 0xff */ MP_INT
	/* }}} */
};

/**
 * This lookup table used by mp_next() and mp_check() to determine
 * size of MsgPack element by its first byte.
 * A positive value contains size of the element (excluding the first byte).
 * A negative value means the element is compound (e.g. array or map)
 * of size (-n).
 * MP_HINT_* values used for special cases handled by switch() statement.
 */
const int8_t mp_parser_hint[256] = {
	/* {{{ MP_UINT(fixed) **/
	/* 0x00 */ 0,
	/* 0x01 */ 0,
	/* 0x02 */ 0,
	/* 0x03 */ 0,
	/* 0x04 */ 0,
	/* 0x05 */ 0,
	/* 0x06 */ 0,
	/* 0x07 */ 0,
	/* 0x08 */ 0,
	/* 0x09 */ 0,
	/* 0x0a */ 0,
	/* 0x0b */ 0,
	/* 0x0c */ 0,
	/* 0x0d */ 0,
	/* 0x0e */ 0,
	/* 0x0f */ 0,
	/* 0x10 */ 0,
	/* 0x11 */ 0,
	/* 0x12 */ 0,
	/* 0x13 */ 0,
	/* 0x14 */ 0,
	/* 0x15 */ 0,
	/* 0x16 */ 0,
	/* 0x17 */ 0,
	/* 0x18 */ 0,
	/* 0x19 */ 0,
	/* 0x1a */ 0,
	/* 0x1b */ 0,
	/* 0x1c */ 0,
	/* 0x1d */ 0,
	/* 0x1e */ 0,
	/* 0x1f */ 0,
	/* 0x20 */ 0,
	/* 0x21 */ 0,
	/* 0x22 */ 0,
	/* 0x23 */ 0,
	/* 0x24 */ 0,
	/* 0x25 */ 0,
	/* 0x26 */ 0,
	/* 0x27 */ 0,
	/* 0x28 */ 0,
	/* 0x29 */ 0,
	/* 0x2a */ 0,
	/* 0x2b */ 0,
	/* 0x2c */ 0,
	/* 0x2d */ 0,
	/* 0x2e */ 0,
	/* 0x2f */ 0,
	/* 0x30 */ 0,
	/* 0x31 */ 0,
	/* 0x32 */ 0,
	/* 0x33 */ 0,
	/* 0x34 */ 0,
	/* 0x35 */ 0,
	/* 0x36 */ 0,
	/* 0x37 */ 0,
	/* 0x38 */ 0,
	/* 0x39 */ 0,
	/* 0x3a */ 0,
	/* 0x3b */ 0,
	/* 0x3c */ 0,
	/* 0x3d */ 0,
	/* 0x3e */ 0,
	/* 0x3f */ 0,
	/* 0x40 */ 0,
	/* 0x41 */ 0,
	/* 0x42 */ 0,
	/* 0x43 */ 0,
	/* 0x44 */ 0,
	/* 0x45 */ 0,
	/* 0x46 */ 0,
	/* 0x47 */ 0,
	/* 0x48 */ 0,
	/* 0x49 */ 0,
	/* 0x4a */ 0,
	/* 0x4b */ 0,
	/* 0x4c */ 0,
	/* 0x4d */ 0,
	/* 0x4e */ 0,
	/* 0x4f */ 0,
	/* 0x50 */ 0,
	/* 0x51 */ 0,
	/* 0x52 */ 0,
	/* 0x53 */ 0,
	/* 0x54 */ 0,
	/* 0x55 */ 0,
	/* 0x56 */ 0,
	/* 0x57 */ 0,
	/* 0x58 */ 0,
	/* 0x59 */ 0,
	/* 0x5a */ 0,
	/* 0x5b */ 0,
	/* 0x5c */ 0,
	/* 0x5d */ 0,
	/* 0x5e */ 0,
	/* 0x5f */ 0,
	/* 0x60 */ 0,
	/* 0x61 */ 0,
	/* 0x62 */ 0,
	/* 0x63 */ 0,
	/* 0x64 */ 0,
	/* 0x65 */ 0,
	/* 0x66 */ 0,
	/* 0x67 */ 0,
	/* 0x68 */ 0,
	/* 0x69 */ 0,
	/* 0x6a */ 0,
	/* 0x6b */ 0,
	/* 0x6c */ 0,
	/* 0x6d */ 0,
	/* 0x6e */ 0,
	/* 0x6f */ 0,
	/* 0x70 */ 0,
	/* 0x71 */ 0,
	/* 0x72 */ 0,
	/* 0x73 */ 0,
	/* 0x74 */ 0,
	/* 0x75 */ 0,
	/* 0x76 */ 0,
	/* 0x77 */ 0,
	/* 0x78 */ 0,
	/* 0x79 */ 0,
	/* 0x7a */ 0,
	/* 0x7b */ 0,
	/* 0x7c */ 0,
	/* 0x7d */ 0,
	/* 0x7e */ 0,
	/* 0x7f */ 0,
	/* }}} */

	/* {{{ MP_MAP (fixed) */
	/* 0x80 */  0, /* empty map - just skip one byte */
	/* 0x81 */ -2, /* 2 elements follow */
	/* 0x82 */ -4,
	/* 0x83 */ -6,
	/* 0x84 */ -8,
	/* 0x85 */ -10,
	/* 0x86 */ -12,
	/* 0x87 */ -14,
	/* 0x88 */ -16,
	/* 0x89 */ -18,
	/* 0x8a */ -20,
	/* 0x8b */ -22,
	/* 0x8c */ -24,
	/* 0x8d */ -26,
	/* 0x8e */ -28,
	/* 0x8f */ -30,
	/* }}} */

	/* {{{ MP_ARRAY (fixed) */
	/* 0x90 */  0,  /* empty array - just skip one byte */
	/* 0x91 */ -1,  /* 1 element follows */
	/* 0x92 */ -2,
	/* 0x93 */ -3,
	/* 0x94 */ -4,
	/* 0x95 */ -5,
	/* 0x96 */ -6,
	/* 0x97 */ -7,
	/* 0x98 */ -8,
	/* 0x99 */ -9,
	/* 0x9a */ -10,
	/* 0x9b */ -11,
	/* 0x9c */ -12,
	/* 0x9d */ -13,
	/* 0x9e */ -14,
	/* 0x9f */ -15,
	/* }}} */

	/* {{{ MP_STR (fixed) */
	/* 0xa0 */ 0,
	/* 0xa1 */ 1,
	/* 0xa2 */ 2,
	/* 0xa3 */ 3,
	/* 0xa4 */ 4,
	/* 0xa5 */ 5,
	/* 0xa6 */ 6,
	/* 0xa7 */ 7,
	/* 0xa8 */ 8,
	/* 0xa9 */ 9,
	/* 0xaa */ 10,
	/* 0xab */ 11,
	/* 0xac */ 12,
	/* 0xad */ 13,
	/* 0xae */ 14,
	/* 0xaf */ 15,
	/* 0xb0 */ 16,
	/* 0xb1 */ 17,
	/* 0xb2 */ 18,
	/* 0xb3 */ 19,
	/* 0xb4 */ 20,
	/* 0xb5 */ 21,
	/* 0xb6 */ 22,
	/* 0xb7 */ 23,
	/* 0xb8 */ 24,
	/* 0xb9 */ 25,
	/* 0xba */ 26,
	/* 0xbb */ 27,
	/* 0xbc */ 28,
	/* 0xbd */ 29,
	/* 0xbe */ 30,
	/* 0xbf */ 31,
	/* }}} */

	/* {{{ MP_NIL, MP_BOOL */
	/* 0xc0 */ 0, /* MP_NIL */
	/* 0xc1 */ 0, /* never used */
	/* 0xc2 */ 0, /* MP_BOOL*/
	/* 0xc3 */ 0, /* MP_BOOL*/
	/* }}} */

	/* {{{ MP_BIN */
	/* 0xc4 */ MP_HINT_STR_8,  /* MP_BIN (8)  */
	/* 0xc5 */ MP_HINT_STR_16, /* MP_BIN (16) */
	/* 0xc6 */ MP_HINT_STR_32, /* MP_BIN (32) */
	/* }}} */

	/* {{{ MP_EXT */
	/* 0xc7 */ MP_HINT_EXT_8,    /* MP_EXT (8)  */
	/* 0xc8 */ MP_HINT_EXT_16,   /* MP_EXT (16) */
	/* 0xc9 */ MP_HINT_EXT_32,   /* MP_EXT (32) */
	/* }}} */

	/* {{{ MP_FLOAT, MP_DOUBLE */
	/* 0xca */ sizeof(float),    /* MP_FLOAT */
	/* 0xcb */ sizeof(double),   /* MP_DOUBLE */
	/* }}} */

	/* {{{ MP_UINT */
	/* 0xcc */ sizeof(uint8_t),  /* MP_UINT (8)  */
	/* 0xcd */ sizeof(uint16_t), /* MP_UINT (16) */
	/* 0xce */ sizeof(uint32_t), /* MP_UINT (32) */
	/* 0xcf */ sizeof(uint64_t), /* MP_UINT (64) */
	/* }}} */

	/* {{{ MP_INT */
	/* 0xd0 */ sizeof(uint8_t),  /* MP_INT (8)  */
	/* 0xd1 */ sizeof(uint16_t), /* MP_INT (8)  */
	/* 0xd2 */ sizeof(uint32_t), /* MP_INT (8)  */
	/* 0xd3 */ sizeof(uint64_t), /* MP_INT (8)  */
	/* }}} */

	/* {{{ MP_EXT (fixext) */
	/* 0xd4 */ 2,  /* MP_EXT (fixext 8)   */
	/* 0xd5 */ 3,  /* MP_EXT (fixext 16)  */
	/* 0xd6 */ 5,  /* MP_EXT (fixext 32)  */
	/* 0xd7 */ 9,  /* MP_EXT (fixext 64)  */
	/* 0xd8 */ 17, /* MP_EXT (fixext 128) */
	/* }}} */

	/* {{{ MP_STR */
	/* 0xd9 */ MP_HINT_STR_8,      /* MP_STR (8) */
	/* 0xda */ MP_HINT_STR_16,     /* MP_STR (16) */
	/* 0xdb */ MP_HINT_STR_32,     /* MP_STR (32) */
	/* }}} */

	/* {{{ MP_ARRAY */
	/* 0xdc */ MP_HINT_ARRAY_16,   /* MP_ARRAY (16) */
	/* 0xdd */ MP_HINT_ARRAY_32,   /* MP_ARRAY (32) */
	/* }}} */

	/* {{{ MP_MAP */
	/* 0xde */ MP_HINT_MAP_16,     /* MP_MAP (16) */
	/* 0xdf */ MP_HINT_MAP_32,     /* MP_MAP (32) */
	/* }}} */

	/* {{{ MP_INT (fixed) */
	/* 0xe0 */ 0,
	/* 0xe1 */ 0,
	/* 0xe2 */ 0,
	/* 0xe3 */ 0,
	/* 0xe4 */ 0,
	/* 0xe5 */ 0,
	/* 0xe6 */ 0,
	/* 0xe7 */ 0,
	/* 0xe8 */ 0,
	/* 0xe9 */ 0,
	/* 0xea */ 0,
	/* 0xeb */ 0,
	/* 0xec */ 0,
	/* 0xed */ 0,
	/* 0xee */ 0,
	/* 0xef */ 0,
	/* 0xf0 */ 0,
	/* 0xf1 */ 0,
	/* 0xf2 */ 0,
	/* 0xf3 */ 0,
	/* 0xf4 */ 0,
	/* 0xf5 */ 0,
	/* 0xf6 */ 0,
	/* 0xf7 */ 0,
	/* 0xf8 */ 0,
	/* 0xf9 */ 0,
	/* 0xfa */ 0,
	/* 0xfb */ 0,
	/* 0xfc */ 0,
	/* 0xfd */ 0,
	/* 0xfe */ 0,
	/* 0xff */ 0
	/* }}} */
};

#endif /* defined(MP_SOURCE) */

/** \endcond */

/*
 * }}}
 */

#if defined(__cplusplus)
} /* extern "C" */
#endif /* defined(__cplusplus) */

#undef MP_LOAD_STORE
#undef MP_SOURCE
#undef MP_PROTO
#undef MP_IMPL
#undef MP_ALWAYSINLINE
#undef MP_GCC_VERSION

#endif /* MSGPUCK_H_INCLUDED */
