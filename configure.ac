AC_INIT(rtorrent, 0.9.4, sundell.software@gmail.com)

AC_DEFINE(API_VERSION, 8, api version)

AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS(config.h)
AM_PATH_CPPUNIT(1.9.6)

AC_PROG_CXX
AC_PROG_LIBTOOL

TORRENT_CHECK_CXXFLAGS()
TORRENT_ENABLE_DEBUG()
TORRENT_ENABLE_EXTRA_DEBUG()
TORRENT_ENABLE_WERROR()
TORRENT_ENABLE_TR1()
TORRENT_ENABLE_CXX11()

TORRENT_DISABLE_IPV6

AC_SYS_LARGEFILE

TORRENT_CHECK_EXECINFO()
TORRENT_OTFD()

TORRENT_ENABLE_ARCH
TORRENT_WITH_SYSROOT

TORRENT_WITHOUT_VARIABLE_FDSET()
TORRENT_WITHOUT_STATVFS()
TORRENT_WITHOUT_STATFS()

AX_PTHREAD([], AC_MSG_ERROR([requires pthread]))
AX_WITH_CURSES()

if test "x$ax_cv_ncursesw" != xyes && test "x$ax_cv_ncurses" != xyes; then
   AC_MSG_ERROR([requires either NcursesW or Ncurses library])
fi

CFLAGS="$CFLAGS $PTHREAD_CFLAGS $CURSES_CFLAGS"
CXXFLAGS="$CXXFLAGS $PTHREAD_CFLAGS $CURSES_CFLAGS"
LIBS="$PTHREAD_LIBS $CURSES_LIB $LIBS"

PKG_CHECK_MODULES([libcurl], libcurl >= 7.15.4,
	          CXXFLAGS="$CXXFLAGS $libcurl_CFLAGS";
		  LIBS="$LIBS $libcurl_LIBS")

PKG_CHECK_MODULES([libtorrent], libtorrent >= 0.13.4,
	          CXXFLAGS="$CXXFLAGS $libtorrent_CFLAGS";
		  LIBS="$LIBS $libtorrent_LIBS")

AC_LANG_PUSH(C++)
TORRENT_WITH_XMLRPC_C
AC_LANG_POP(C++)

AC_DEFINE(HAVE_CONFIG_H, 1, true if config.h was included)
AC_DEFINE(USER_AGENT, [std::string(PACKAGE "/" VERSION "/") + torrent::version()], Http user agent)

AC_CHECK_FUNCS(posix_memalign)
TORRENT_CHECK_CACHELINE()
TORRENT_CHECK_POPCOUNT()

CC_ATTRIBUTE_UNUSED(
	AC_DEFINE([__UNUSED], [__attribute__((unused))], [Wrapper around unused attribute]),
	AC_DEFINE([__UNUSED], [], [Null-wrapper if unused attribute is unsupported])
)

AC_OUTPUT([
	Makefile
	doc/Makefile
	src/Makefile
	src/core/Makefile
	src/display/Makefile
	src/input/Makefile
	src/rpc/Makefile
	src/ui/Makefile
	src/utils/Makefile
        test/Makefile
])
