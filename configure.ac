m4_pattern_allow([PKG_CHECK_EXISTS])

AC_INIT([rtorrent],[0.16.15],[sundell.software@gmail.com])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([scripts])
AM_INIT_AUTOMAKE([foreign subdir-objects silent-rules])
AM_PROG_AR
AC_PROG_RANLIB

AC_PROG_CXX
AX_CXX_COMPILE_STDCXX(20, noext, mandatory)

PKG_PROG_PKG_CONFIG

# ── libtorrent peer identity ──
AC_DEFINE([PEER_NAME], [["-lt100E-"]], [Identifier that is part of the default peer id.])
AC_DEFINE([PEER_VERSION], [["lt\x10\x0E"]], [4 byte client and version identifier for DHT.])

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

# ── libtorrent feature checks ──
TORRENT_CHECK_ATOMIC
TORRENT_CHECK_CUSTOM_ENDIAN64
TORRENT_CHECK_MADVISE
TORRENT_CHECK_POSIX_FADVISE

TORRENT_WITHOUT_KQUEUE
TORRENT_WITHOUT_EPOLL

if test "x$use_kqueue" != "xyes" && test "x$use_epoll" != "xyes"; then
  AC_MSG_ERROR([Must enable at least one of either kqueue or epoll])
fi

TORRENT_WITH_ADDRESS_SPACE
TORRENT_WITH_INOTIFY

AC_ARG_ENABLE(attribute-visibility,
  AS_HELP_STRING([--disable-attribute-visibility],[disable symbol visibility attribute [[default=enable]]]),
  [
    if test "$enableval" = "yes"; then
      CC_ATTRIBUTE_VISIBILITY
    fi
  ],[
    CC_ATTRIBUTE_VISIBILITY
  ])

AC_CHECK_FUNCS([fallocate posix_fallocate])
TORRENT_ENABLE_CUSTOM_STACK_SIZE

PKG_CHECK_MODULES([LIBCURL], [libcurl],, [LIBCURL_CHECK_CONFIG])

# ── rtorrent feature checks ──
TORRENT_WITHOUT_NCURSES

if test "x$with_ncurses" != xno; then
  AX_WITH_CURSES

  if test "x$ax_cv_ncursesw" != xyes && test "x$ax_cv_ncurses" != xyes; then
    AC_MSG_ERROR([requires either NcursesW or Ncurses library])
  fi
fi

PKG_CHECK_MODULES([CPPUNIT], [cppunit],, [no_cppunit="yes"])
PKG_CHECK_MODULES([ZLIB], [zlib])
TORRENT_CHECK_OPENSSL
TORRENT_DISABLE_PTHREAD_SETNAME_NP
TORRENT_MINCORE
TORRENT_ENABLE_INSTRUMENTATION

# ── in-tree libtorrent (monorepo) ──
libtorrent_src="$srcdir/libtorrent"

if test ! -f "$libtorrent_src/src/manager.cc"; then
  AC_MSG_ERROR([libtorrent not found at $libtorrent_src.\n\
  This is a monorepo build — libtorrent must be a submodule in libtorrent/.])
fi

AC_MSG_NOTICE([using in-tree libtorrent at $libtorrent_src])

DEPENDENCIES_CFLAGS="-I\$(top_srcdir)/libtorrent -I\$(top_srcdir)/libtorrent/src"
AC_SUBST([DEPENDENCIES_CFLAGS])

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

AC_DEFINE([HAVE_CONFIG_H], [1], [true if config.h was included])

dnl Only update global build variables immediately before generating the output,
dnl to avoid affecting the global build environment for other autoconf checks.
LIBS="$LIBCURL_LIBS $ATOMIC_LIBS $PTHREAD_LIBS $CURSES_LIB $CURSES_LIBS $ZLIB_LIBS $LIBS"
CFLAGS="$LIBCURL_CFLAGS $PTHREAD_CFLAGS $CURSES_CFLAGS $ZLIB_CFLAGS $DEPENDENCIES_CFLAGS $CFLAGS"
CXXFLAGS="$LIBCURL_CFLAGS $LIBCURL_CPPFLAGS $PTHREAD_CFLAGS $CURSES_CFLAGS $ZLIB_CFLAGS $DEPENDENCIES_CFLAGS $CXXFLAGS"

TORRENT_CHECK_CACHELINE

AC_CONFIG_FILES([
  Makefile
  doc/Makefile
  src/Makefile
  test/Makefile
  test/libtorrent/Makefile
])

AC_OUTPUT
