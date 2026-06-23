SUBDIRS = \
	doc \
	src \
	test

noinst_LIBRARIES = librtorrent.a

include libtorrent_sources.am
include rtorrent_sources.am

if NO_NCURSES
librtorrent_a_SOURCES += src/display/curses_stub.cc src/display/curses_stub.h
endif

# Ensure the convenience library is built before recursing into SUBDIRS.
# (automake normally does this via all-am, but we make it explicit.)
all: all-am

librtorrent_a_CPPFLAGS = \
	-I$(srcdir)/libtorrent \
	-I$(srcdir)/libtorrent/src \
	-I$(srcdir)/libtorrent/src/torrent \
	-I$(srcdir)/src \
	-I$(top_builddir) \
	'-DPACKAGE_DATADIR="$(pkgdatadir)"'

nobase_dist_pkgdata_DATA = \
	lua/rtorrent.lua

EXTRA_DIST= \
	libtorrent_sources.am \
	rtorrent_sources.am \
	scripts/checks.m4 \
	scripts/common.m4 \
	scripts/attributes.m4 \
	scripts/ci/check-api-boundary.sh

ACLOCAL_AMFLAGS = -I scripts
