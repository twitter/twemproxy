#!/usr/bin/env bash
# Main ci script for nutcracker tests
set -xeu

function print_usage() {
    echo "Usage: $0 [REDIS_VER]" 1>&2
    echo "e.g.   $0 3.2.12" 1>&2
    exit 1
}

REDIS_VER=3.2.11
if [[ "$#" > 1 ]]; then
    echo "Too many arguments" 1>&2
    print_usage
elif [[ "$#" > 0 ]]; then
	REDIS_VER="$1"
fi

PACKAGE_NAME="nutcrackerci"

TAG=$( git describe --always )
DOCKER_IMG_NAME=twemproxy-build-$PACKAGE_NAME-$REDIS_VER-$TAG

rm -rf twemproxy

DOCKER_TAG=twemproxy-$PACKAGE_NAME-$REDIS_VER:$TAG

docker build -f ci/Dockerfile \
   --tag $DOCKER_TAG \
   --build-arg=REDIS_VER=$REDIS_VER \
   .

# Run nose tests
docker run \
   --rm \
   -e REDIS_VER=$REDIS_VER \
   --name=$DOCKER_IMG_NAME \
   $DOCKER_TAG \
   nosetests -v test_memcache test_system test_redis
