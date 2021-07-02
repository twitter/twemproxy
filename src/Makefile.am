MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS =
if !OS_SOLARIS
AM_CPPFLAGS += -D_GNU_SOURCE
endif
AM_CPPFLAGS += -I $(top_srcdir)/src/hashkit
AM_CPPFLAGS += -I $(top_srcdir)/src/proto
AM_CPPFLAGS += -I $(top_srcdir)/src/event
AM_CPPFLAGS += -I $(top_srcdir)/contrib/yaml-0.2.5/include

AM_CFLAGS =
# about -fno-strict-aliasing: https://github.com/twitter/twemproxy/issues/276
AM_CFLAGS += -fno-strict-aliasing
AM_CFLAGS += -Wall -Wshadow
AM_CFLAGS += -Wpointer-arith
AM_CFLAGS += -Winline
AM_CFLAGS += -Wunused-function -Wunused-variable -Wunused-value
AM_CFLAGS += -Wno-unused-parameter -Wno-unused-value
AM_CFLAGS += -Wconversion -Wsign-compare
AM_CFLAGS += -Wstrict-prototypes -Wmissing-prototypes -Wredundant-decls -Wmissing-declarations
AM_CFLAGS += -Wno-format-zero-length

AM_LDFLAGS =
AM_LDFLAGS += -lm -lpthread -rdynamic
if OS_SOLARIS
AM_LDFLAGS += -lnsl -lsocket
endif
if OS_FREEBSD
AM_LDFLAGS += -lexecinfo
endif

SUBDIRS = hashkit proto event

sbin_PROGRAMS = nutcracker

nutcracker_SOURCES =			\
	nc_core.c nc_core.h		\
	nc_connection.c nc_connection.h	\
	nc_client.c nc_client.h		\
	nc_server.c nc_server.h		\
	nc_proxy.c nc_proxy.h		\
	nc_message.c nc_message.h	\
	nc_request.c			\
	nc_response.c			\
	nc_mbuf.c nc_mbuf.h		\
	nc_conf.c nc_conf.h		\
	nc_stats.c nc_stats.h		\
	nc_signal.c nc_signal.h		\
	nc_rbtree.c nc_rbtree.h		\
	nc_log.c nc_log.h		\
	nc_string.c nc_string.h		\
	nc_array.c nc_array.h		\
	nc_util.c nc_util.h		\
	nc_queue.h			\
	nc.c

nutcracker_LDADD = $(top_builddir)/src/hashkit/libhashkit.a
nutcracker_LDADD += $(top_builddir)/src/proto/libproto.a
nutcracker_LDADD += $(top_builddir)/src/event/libevent.a
nutcracker_LDADD += $(top_builddir)/contrib/yaml-0.2.5/src/.libs/libyaml.a

TESTS = test_all
bin_PROGRAMS = test_all

test_all_SOURCES = test_all.c \
	nc_core.c nc_core.h		\
	nc_connection.c nc_connection.h	\
	nc_client.c nc_client.h		\
	nc_server.c nc_server.h		\
	nc_proxy.c nc_proxy.h		\
	nc_message.c nc_message.h	\
	nc_request.c			\
	nc_response.c			\
	nc_mbuf.c nc_mbuf.h		\
	nc_conf.c nc_conf.h		\
	nc_stats.c nc_stats.h		\
	nc_signal.c nc_signal.h		\
	nc_rbtree.c nc_rbtree.h		\
	nc_log.c nc_log.h		\
	nc_string.c nc_string.h		\
	nc_array.c nc_array.h		\
	nc_util.c nc_util.h		\
	nc_queue.h

test_all_LDADD = $(top_builddir)/src/hashkit/libhashkit.a
test_all_LDADD += $(top_builddir)/src/proto/libproto.a
test_all_LDADD += $(top_builddir)/src/event/libevent.a
test_all_LDADD += $(top_builddir)/contrib/yaml-0.2.5/src/.libs/libyaml.a
