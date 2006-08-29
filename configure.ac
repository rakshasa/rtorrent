AC_INIT(rtorrent, 0.6.2, jaris@ifi.uio.no)

AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)

TORRENT_CHECK_CXXFLAGS()
TORRENT_ENABLE_DEBUG()
TORRENT_ENABLE_EXTRA_DEBUG()
TORRENT_ENABLE_WERROR()

TORRENT_DISABLE_IPV6

AC_PROG_CXX
AC_PROG_LIBTOOL
AC_SYS_LARGEFILE

TORRENT_CHECK_EXECINFO()
TORRENT_CHECK_CURL()
TORRENT_OTFD()

TORRENT_WITHOUT_VARIABLE_FDSET()
TORRENT_WITHOUT_NCURSESW()
TORRENT_WITHOUT_STATVFS()
TORRENT_WITHOUT_STATFS()

PKG_CHECK_MODULES(STUFF, sigc++-2.0 libtorrent >= 0.10.1,
	          CXXFLAGS="$CXXFLAGS $STUFF_CFLAGS $CURL_CFLAGS";
		  LIBS="$LIBS $STUFF_LIBS $CURL_LIBS")

AC_DEFINE(HAVE_CONFIG_H, 1, true if config.h was included)
AC_DEFINE(USER_AGENT, [std::string(PACKAGE "/" VERSION "/") + torrent::version()], Http user agent)

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
	src/ui/Makefile
	src/utils/Makefile
])
