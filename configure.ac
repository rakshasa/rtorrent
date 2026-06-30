m4_pattern_allow([PKG_CHECK_EXISTS])

AC_INIT([rtorrent],[0.16.15],[sundell.software@gmail.com])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([scripts])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_PROG_AR

LT_INIT

AC_PROG_CXX
AX_CXX_COMPILE_STDCXX(20, noext, mandatory)

PKG_PROG_PKG_CONFIG

AC_DEFINE([API_VERSION], [23], [api version])

RAK_CHECK_CFLAGS
RAK_CHECK_CXXFLAGS
RAK_ENABLE_DEBUG
RAK_ENABLE_EXTRA_DEBUG
RAK_ENABLE_WERROR

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

TORRENT_WITHOUT_NCURSES

if test "x$with_ncurses" != xno; then
  AX_WITH_CURSES

  if test "x$ax_cv_ncursesw" != xyes && test "x$ax_cv_ncurses" != xyes; then
    AC_MSG_ERROR([requires either NcursesW or Ncurses library])
  fi
fi

PKG_CHECK_MODULES([CPPUNIT], [cppunit],, [no_cppunit="yes"])
PKG_CHECK_MODULES([ZLIB], [zlib])
PKG_CHECK_MODULES([DEPENDENCIES], [libtorrent >= 0.16.15])

AC_ARG_ENABLE(webtorrent,
  AS_HELP_STRING([--disable-webtorrent],
    [disable WebTorrent support [[default=enable]]]),
  [], [enable_webtorrent=yes])

AS_IF([test "x$enable_webtorrent" != "xno"], [
  AC_LANG_PUSH([C++])
  rtorrent_save_CXXFLAGS="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS $DEPENDENCIES_CFLAGS"
  AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
      [[
        #include <torrent/common.h>
        #include <torrent/peer/peer.h>
        #include <torrent/runtime/runtime.h>
        #ifndef LIBTORRENT_HAS_WEBTORRENT
        #error "libtorrent was not built with WebTorrent support"
        #endif
      ]],
      [[
        bool supported = torrent::runtime::webtorrent_supported();
        torrent::runtime::set_webtorrent_enabled(supported);
        bool enabled = torrent::runtime::webtorrent_enabled();
        torrent::tracker_enum tracker_type = torrent::TRACKER_WEBSOCKET;
        torrent::Peer* peer = nullptr;
        bool is_webtorrent = peer != nullptr && peer->is_webtorrent();
        (void)enabled;
        (void)tracker_type;
        (void)is_webtorrent;
      ]])],
    [AC_DEFINE([HAVE_LIBUTORRENT_WEBTORRENT], [1], [Define if libtorrent exposes WebTorrent support.])],
    [AC_MSG_ERROR([WebTorrent support requested but installed libtorrent does not expose the required WebTorrent API. Install a WebTorrent-enabled rakshasa/libtorrent or configure with --disable-webtorrent.])])
  CXXFLAGS="$rtorrent_save_CXXFLAGS"
  AC_LANG_POP([C++])
])

AC_LANG_PUSH(C++)
TORRENT_WITH_XMLRPC_C
AC_LANG_POP(C++)

dnl We don't have LuaJIT support, and the script breaks if not disabled.
AM_CONDITIONAL([LUAJIT], [false])

TORRENT_WITH_LUA
TORRENT_WITH_TINYXML2
TORRENT_WITH_SYSTEMD

if test ${with_xmlrpc_c+y} && test ${with_xmlrpc_tinyxml2+y}; then
  AC_MSG_ERROR([--with-xmlrpc-c and --with-xmlrpc-tinyxml2 cannot be used together. Please choose only one])
fi

AC_DEFINE(USER_AGENT, [std::string(PACKAGE "/" VERSION)], Http user agent)

dnl Only update global build variables immediately before generating the output,
dnl to avoid affecting the global build environment for other autoconf checks.
LIBS="$PTHREAD_LIBS $CURSES_LIB $CURSES_LIBS $ZLIB_LIBS $DEPENDENCIES_LIBS $LIBS"
CFLAGS="$CFLAGS $PTHREAD_CFLAGS $CURSES_CFLAGS $ZLIB_CFLAGS $DEPENDENCIES_CFLAGS"
CXXFLAGS="$CXXFLAGS $PTHREAD_CFLAGS $CURSES_CFLAGS $ZLIB_CFLAGS $DEPENDENCIES_CFLAGS"

TORRENT_CHECK_CACHELINE

AC_CONFIG_FILES([
  Makefile
  doc/Makefile
  src/Makefile
  test/Makefile
])

AC_OUTPUT
