#!/usr/bin/env bash

set -xeu
function cleanup {
    rm -f *.gz
}
trap cleanup EXIT
trap cleanup INT

cleanup

export CFLAGS="-O3 -fno-strict-aliasing -I/usr/lib/x86_64-redhat-linux6E/include -B /usr/lib/x86_64-redhat-linux6E/lib64"
# TODO: Figure out how to make this apply only to the contrib/ directory. Maybe override the yaml directory's Makefile.am after extracting it.
# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53119
CFLAGS+=" -Werror -Wall -Wno-pointer-sign -Wno-sign-conversion -Wno-missing-braces -Wno-unused-value -Wno-builtin-declaration-mismatch -Wno-maybe-uninitialized"
export LDFLAGS="-lc_nonshared"
cd /usr/src/twemproxy
autoreconf -fvi
./configure --enable-debug=yes --prefix=/usr/src/twemproxy/work/usr
make -j5
make install
