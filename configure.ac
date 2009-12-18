AC_INIT(rtorrent, 0.8.6, jaris@ifi.uio.no)

AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)

AC_PROG_CXX
AC_PROG_LIBTOOL

TORRENT_CHECK_CXXFLAGS()
TORRENT_ENABLE_DEBUG()
TORRENT_ENABLE_EXTRA_DEBUG()
TORRENT_ENABLE_WERROR()

TORRENT_DISABLE_IPV6

AC_SYS_LARGEFILE

TORRENT_CHECK_EXECINFO()
TORRENT_OTFD()

TORRENT_ENABLE_ARCH
TORRENT_WITH_SYSROOT

TORRENT_WITHOUT_VARIABLE_FDSET()
TORRENT_WITHOUT_NCURSESW()
TORRENT_WITHOUT_STATVFS()
TORRENT_WITHOUT_STATFS()

PKG_CHECK_MODULES(sigc, sigc++-2.0,
	          CXXFLAGS="$CXXFLAGS $sigc_CFLAGS";
		  LIBS="$LIBS $sigc_LIBS")

PKG_CHECK_MODULES(libcurl, libcurl >= 7.15.4,
	          CXXFLAGS="$CXXFLAGS $libcurl_CFLAGS";
		  LIBS="$LIBS $libcurl_LIBS")

PKG_CHECK_MODULES(libtorrent, libtorrent >= 0.12.6,
	          CXXFLAGS="$CXXFLAGS $libtorrent_CFLAGS";
		  LIBS="$LIBS $libtorrent_LIBS")

AC_LANG_PUSH(C++)
TORRENT_WITH_XMLRPC_C
AC_LANG_POP(C++)

AC_DEFINE(HAVE_CONFIG_H, 1, true if config.h was included)
AC_DEFINE(USER_AGENT, [std::string(PACKAGE "/" VERSION "/") + torrent::version()], Http user agent)

AC_CHECK_FUNC(posix_memalign)
TORRENT_CHECK_CACHELINE()

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
])
