#ifndef BASE64_H
#define BASE64_H
/*
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
/*
 * This is part of the libb64 project, and has been placed in the
 * public domain. For details, see
 * http://sourceforge.net/projects/libb64
 */
#ifdef __cplusplus
extern "C" {
#endif

#define BASE64_CHARS_PER_LINE 72

static inline int
base64_bufsize(int binsize)
{
        int datasize = binsize * 4/3 + 4;
        int newlines = ((datasize + BASE64_CHARS_PER_LINE - 1)/
                        BASE64_CHARS_PER_LINE);
        return datasize + newlines;
}

/**
 * Encode a binary stream into BASE64 text.
 *
 * @pre the buffer size is at least 4/3 of the stream
 * size + stream_size/72 (newlines) + 4
 *
 * @param[in]  in_bin           the binary input stream to decode
 * @param[in]  in_len		size of the input
 * @param[out] out_base64       output buffer for the encoded data
 * @param[in]  out_len          buffer size, must be at least
 *				4/3 of the input size
 *
 * @return the size of encoded output
 */

int
base64_encode(const char *in_bin, int in_len,
              char *out_base64, int out_len);

/**
 * Decode a BASE64 text into a binary
 *
 * @param[in]  in_base64	the BASE64 stream to decode
 * @param[in]  in_len		size of the input
 * @param[out] out_bin		output buffer size
 * @param[in]  out_len		buffer size
 *
 * @pre the output buffer size must be at least
 * 3/4 + 1 of the size of the input
 *
 * @return the size of decoded output
 */

int base64_decode(const char *in_base64, int in_len,
                  char *out_bin, int out_len);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* BASE64_H */

