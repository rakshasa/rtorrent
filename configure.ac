m4_pattern_allow([PKG_CHECK_EXISTS])

AC_INIT([rtorrent],[0.16.2],[sundell.software@gmail.com])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([scripts])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_PROG_AR

LT_INIT

AC_PROG_CXX

AC_DEFINE([API_VERSION], [18], [api version])

# Filter out unwanted flags added by autoconf on some systems, e.g. MacOS.
TORRENT_REMOVE_UNWANTED(CXX, $CXX, -std=c++11 -std=gnu++11)

AX_CXX_COMPILE_STDCXX([17], [noext], [mandatory])

AC_SYS_LARGEFILE

RAK_CHECK_CFLAGS
RAK_CHECK_CXXFLAGS
RAK_ENABLE_DEBUG
RAK_ENABLE_EXTRA_DEBUG
RAK_ENABLE_WERROR

TORRENT_ENABLE_ARCH
TORRENT_WITH_SYSROOT

AC_ARG_ENABLE(execinfo,
  AS_HELP_STRING([--disable-execinfo],
    [disable libexecinfo [[default=enable]]]),
  [
    if test "$enableval" = "yes"; then
      AX_EXECINFO
    fi
  ],[
    AX_EXECINFO
  ])

AC_ARG_ENABLE([headless],
  AS_HELP_STRING([--enable-headless], [Enable headless build])
)
AS_IF([test "x$enable_headless" = "xyes"], [
  AC_DEFINE([HEADLESS], [1], [headless build])
  AC_MSG_NOTICE([Enabling HEADLESS build])
])
AM_CONDITIONAL([ENABLE_UI], [test "x$enable_headless" != "xyes"])

AX_PTHREAD([], AC_MSG_ERROR([requires pthread]))
AS_IF([test "x$enable_headless" != "xyes"], [
    AX_WITH_CURSES
    AS_IF([test "x$ax_cv_ncursesw" != xyes && test "x$ax_cv_ncurses" != xyes], [
        AC_MSG_ERROR([requires either NcursesW or Ncurses library])
    ])
])

PKG_CHECK_MODULES([CPPUNIT], [cppunit],, [no_cppunit="yes"])
PKG_CHECK_MODULES([DEPENDENCIES], [libtorrent >= 0.16.2])

AC_LANG_PUSH(C++)
TORRENT_WITH_XMLRPC_C
AC_LANG_POP(C++)

TORRENT_WITH_LUA
TORRENT_WITH_TINYXML2

AS_IF([test "x$with_xmlrpc_c" = "xyes" && test "x$with_xmlrpc_tinyxml2" = "xyes"], [
  AC_MSG_ERROR([--with-xmlrpc-c and --with-xmlrpc-tinyxml2 cannot be used together. Please choose only one])
])
AS_IF([test "x$with_xmlrpc_c" != "xyes" && test "x$with_xmlrpc_tinyxml2" != "xyes" && test "x$enable_headless" = "xyes"], [
  AC_MSG_ERROR([--enable-headless without xmlrpc is useless. Please choose --with-xmlrpc-c or --with-xmlrpc-tinyxml2])
])

AC_DEFINE(USER_AGENT, [std::string(PACKAGE "/" VERSION "/") + torrent::version()], Http user agent)

dnl Only update global build variables immediately before generating the output,
dnl to avoid affecting the global build environment for other autoconf checks.
LIBS="$PTHREAD_LIBS $CURSES_LIB $CURSES_LIBS $DEPENDENCIES_LIBS $LIBS"
CFLAGS="$CFLAGS $PTHREAD_CFLAGS $DEPENDENCIES_CFLAGS $CURSES_CFLAGS"
CXXFLAGS="$CXXFLAGS $PTHREAD_CFLAGS $DEPENDENCIES_CFLAGS $CURSES_CFLAGS"

TORRENT_CHECK_POPCOUNT()

AC_CONFIG_FILES([
  Makefile
  doc/Makefile
  src/Makefile
  test/Makefile
])

AC_OUTPUT
