# Define the package version numbers and the bug reporting address
m4_define([NC_MAJOR], 0)
m4_define([NC_MINOR], 2)
m4_define([NC_PATCH], 4)
m4_define([NC_BUGS], [manj@twitter.com])

# Initialize autoconf
AC_PREREQ([2.64])
AC_INIT([nutcracker], [NC_MAJOR.NC_MINOR.NC_PATCH], [NC_BUGS])
AC_CONFIG_SRCDIR([src/nc.c])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_HEADERS([config.h:config.h.in])
AC_CONFIG_MACRO_DIR([m4])

# Initialize automake
AM_INIT_AUTOMAKE([1.9 foreign])

# Define macro variables for the package version numbers
AC_DEFINE(NC_VERSION_MAJOR, NC_MAJOR, [Define the major version number])
AC_DEFINE(NC_VERSION_MINOR, NC_MINOR, [Define the minor version number])
AC_DEFINE(NC_VERSION_PATCH, NC_PATCH, [Define the patch version number])
AC_DEFINE(NC_VERSION_STRING, "NC_MAJOR.NC_MINOR.NC_PATCH", [Define the version string])

# Checks for language
AC_LANG([C])

# Checks for programs
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_LIBTOOL

# Checks for typedefs, structures, and compiler characteristics
AC_C_INLINE
AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INTMAX_T
AC_TYPE_INTPTR_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINTMAX_T
AC_TYPE_UINTPTR_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

AC_C_BIGENDIAN(
  [],
  [AC_DEFINE(HAVE_LITTLE_ENDIAN, 1, [Define to 1 if machine is little endian])],
  [AC_MSG_ERROR([endianess of this machine is unknown])],
  [AC_MSG_ERROR([universial endianess not supported])]
)

# Checks for header files
AC_HEADER_STDBOOL
AC_CHECK_HEADERS([fcntl.h float.h limits.h stddef.h stdlib.h string.h unistd.h])
AC_CHECK_HEADERS([inttypes.h stdint.h])
AC_CHECK_HEADERS([sys/ioctl.h sys/time.h sys/uio.h])
AC_CHECK_HEADERS([sys/socket.h sys/un.h netinet/in.h arpa/inet.h netdb.h])
AC_CHECK_HEADERS([sys/epoll.h], [], [AC_MSG_ERROR([required sys/epoll.h header file is missing])])

# Checks for libraries
AC_CHECK_LIB([m], [pow])
AC_CHECK_LIB([pthread], [pthread_create])

# Checks for library functions
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([dup2 gethostname gettimeofday strerror])
AC_CHECK_FUNCS([socket])
AC_CHECK_FUNCS([memchr memmove memset])
AC_CHECK_FUNCS([strchr strndup strtoul])

AC_CACHE_CHECK([if epoll works], [ac_cv_epoll_works],
  AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
int
main(int argc, char **argv)
{
    int fd;

    fd = epoll_create(256);
    if (fd < 0) {
        perror("epoll_create:");
        exit(1);
    }
    exit(0);
}
  ], [ac_cv_epoll_works=yes], [ac_cv_epoll_works=no]))
AS_IF([test "x$ac_cv_epoll_works" = "xyes"], [], [AC_MSG_FAILURE([Linux epoll(7) API is missing])])

# Package options
AC_MSG_CHECKING([whether to enable debug logs and asserts])
AC_ARG_ENABLE([debug],
  [AS_HELP_STRING(
    [--enable-debug=@<:@full|yes|log|no@:>@],
    [enable debug logs and asserts @<:@default=no@:>@])
  ],
  [],
  [enable_debug=no])
AS_CASE([x$enable_debug],
  [xfull], [AC_DEFINE([HAVE_ASSERT_PANIC], [1],
                      [Define to 1 if panic on an assert is enabled])
            AC_DEFINE([HAVE_DEBUG_LOG], [1], [Define to 1 if debug log is enabled])
           ],
  [xyes], [AC_DEFINE([HAVE_ASSERT_LOG], [1],
                     [Define to 1 if log on an assert is enabled])
           AC_DEFINE([HAVE_DEBUG_LOG], [1], [Define to 1 if debug log is enabled])
          ],
  [xlog], [AC_DEFINE([HAVE_DEBUG_LOG], [1], [Define to 1 if debug log is enabled])],
  [xno], [],
  [AC_MSG_FAILURE([invalid value ${enable_debug} for --enable-debug])])
AC_MSG_RESULT($enable_debug)

AC_MSG_CHECKING([whether to disable stats])
AC_ARG_ENABLE([stats],
  [AS_HELP_STRING(
    [--disable-stats],
    [disable stats])
  ],
  [disable_stats=yes],
  [disable_stats=no])
AS_IF([test "x$disable_stats" = xyes],
  [],
  [AC_DEFINE([HAVE_STATS], [1], [Define to 1 if stats is not disabled])])
AC_MSG_RESULT($disable_stats)

# Untar the yaml-0.1.4 in contrib/ before config.status is rerun
AC_CONFIG_COMMANDS_PRE([tar xvfz contrib/yaml-0.1.4.tar.gz -C contrib])

# Call yaml-0.1.4 ./configure recursively
AC_CONFIG_SUBDIRS([contrib/yaml-0.1.4])

# Define Makefiles
AC_CONFIG_FILES([Makefile
                 contrib/Makefile
                 src/Makefile
                 src/hashkit/Makefile
                 src/proto/Makefile])

# Generate the "configure" script
AC_OUTPUT
