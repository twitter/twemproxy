#!/usr/bin/env bash
set -xeu
function cleanup {
    rm -f *.gz
}
trap cleanup EXIT
trap cleanup INT

cleanup

export CFLAGS="-O3 -fno-strict-aliasing -I/usr/lib/x86_64-redhat-linux6E/include -B /usr/lib/x86_64-redhat-linux6E/lib64"
export LDFLAGS="-lc_nonshared"
cd /usr/src/twemproxy
autoreconf -fvi
./configure --enable-debug=log --prefix=/usr/src/twemproxy/work/usr
make -j5
make install
