/* Copied with modification from https://github.com/tarantool/tarantool */

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
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>

#include <nc_core.h>
#include <nc_prng.h>

static int rfd;
static bool initialized = false;

void
nc_prng_init(void)
{
    unsigned seed;
    rfd = open("/dev/urandom", O_RDONLY);
    if (rfd == -1)
        rfd = open("/dev/random", O_RDONLY | O_NONBLOCK);
    if (rfd == -1) {
        struct timeval tv;
        gettimeofday(&tv, 0);
        seed = (unsigned) (((unsigned)getpid() << 16) ^ getuid() ^
                           tv.tv_sec ^ tv.tv_usec);
        goto srand;
    }

    int flags = fcntl(rfd, F_GETFD);
    if (flags != -1)
        fcntl(rfd, F_SETFD, flags | FD_CLOEXEC);

    ssize_t res = read(rfd, &seed, sizeof(seed));
    (void) res;
srand:
    srandom(seed);
    srand(seed);

    initialized = true;
}

void
nc_prng_done(void)
{
    ASSERT(initialized);

    if (rfd == -1)
        return;
    close(rfd);
}

void
nc_prng_fill(char *buf, size_t size)
{
    size_t generated = 0;

    ASSERT(initialized);

    if (rfd == -1)
        goto rand;

    int attempt = 0;
    while (generated < size) {
        ssize_t n = read(rfd, buf + generated, size - generated);
        if (n <= 0) {
            if (attempt++ > 5)
                break;
            continue;
        }
        generated += (size_t)n;
        attempt = 0;
    }
rand:
    /* fill remaining bytes with PRNG */
    generated -= generated % sizeof(int);
    while (generated < size) {
        *(int *)(buf + generated) = rand();
        generated += sizeof(int);
    }
}
