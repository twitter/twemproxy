contrib_gtest_dir = $(top_srcdir)/contrib/gtest-1.7.0
AM_CPPFLAGS = -I$(contrib_gtest_dir)/include -I$(top_srcdir)/src $(GTEST_CPPFLAGS)
AM_CXXFLAGS = $(GTEST_CXXFLAGS)
AM_LDFLAGS = $(GTEST_LDFLAGS)


# TESTS -- Programs run automatically by "make check"
# check_PROGRAMS -- Programs built by "make check" but not necessarily run
TESTS = 
check_PROGRAMS = 

TESTS += test_hashkit
check_PROGRAMS += test_hashkit
test_hashkit_SOURCES = test_hashkit.cpp
test_hashkit_LDADD = $(top_builddir)/src/hashkit/libhashkit.a \
	$(contrib_gtest_dir)/lib/libgtest_main.la \
	$(contrib_gtest_dir)/lib/libgtest.la
