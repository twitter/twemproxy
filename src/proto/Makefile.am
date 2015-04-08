MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = -I $(top_srcdir)/src

AM_CFLAGS = -Wall -Wshadow
AM_CFLAGS += -Wno-unused-parameter -Wno-unused-value

noinst_LIBRARIES = libproto.a

noinst_HEADERS = nc_proto.h base64.h

libproto_a_SOURCES =			\
	nc_memcache.c			\
	nc_redis.c			\
	nc_tarantool.c                  \
	base64.c
