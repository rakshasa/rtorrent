m4_pattern_allow([PKG_CHECK_EXISTS])

AC_INIT([rtorrent],[0.16.5],[sundell.software@gmail.com])

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

AX_PTHREAD([], AC_MSG_ERROR([requires pthread]))
AX_WITH_CURSES

if test "x$ax_cv_ncursesw" != xyes && test "x$ax_cv_ncurses" != xyes; then
  AC_MSG_ERROR([requires either NcursesW or Ncurses library])
fi

PKG_CHECK_MODULES([CPPUNIT], [cppunit],, [no_cppunit="yes"])
PKG_CHECK_MODULES([DEPENDENCIES], [libtorrent >= 0.16.5])

AC_LANG_PUSH(C++)
TORRENT_WITH_XMLRPC_C
AC_LANG_POP(C++)

TORRENT_WITH_LUA
TORRENT_WITH_TINYXML2

if test ${with_xmlrpc_c+y} && test ${with_xmlrpc_tinyxml2+y}; then
  AC_MSG_ERROR([--with-xmlrpc-c and --with-xmlrpc-tinyxml2 cannot be used together. Please choose only one])
fi

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
