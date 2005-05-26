AC_INIT(rtorrent, 0.2.3, jaris@ifi.uio.no)

AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)

sinclude(scripts/checks.m4)
sinclude(scripts/common.m4)

TORRENT_CHECK_CXXFLAGS()
TORRENT_ENABLE_DEBUG()
TORRENT_ENABLE_WERROR()

AC_PROG_CXX
AC_PROG_RANLIB
AC_SYS_LARGEFILE

AC_SEARCH_LIBS(wbkgdset, ncurses curses,,echo "*** The ncurses library is required!";exit 1)

TORRENT_CHECK_EXECINFO()
TORRENT_CHECK_CURL()
TORRENT_OTFD()

PKG_CHECK_MODULES(STUFF, sigc++-2.0 libtorrent >= 0.6.3,
	          CXXFLAGS="$CXXFLAGS $STUFF_CFLAGS $CURL_CFLAGS";
		  LIBS="$LIBS $STUFF_LIBS $CURL_LIBS")

AC_DEFINE(HAVE_CONFIG_H, 1, true if config.h was included)

AC_OUTPUT([
	Makefile
	src/Makefile
	src/core/Makefile
	src/display/Makefile
	src/input/Makefile
	src/ui/Makefile
	src/utils/Makefile
])
