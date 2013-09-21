MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = -I $(top_srcdir)/src

AM_CFLAGS = -Wall -Wshadow
AM_CFLAGS += -Wno-unused-parameter -Wno-unused-value

noinst_LIBRARIES = libevent.a

noinst_HEADERS = nc_event.h

libevent_a_SOURCES =	\
	nc_epoll.c	\
	nc_kqueue.c	\
	nc_evport.c

