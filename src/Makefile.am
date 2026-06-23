bin_PROGRAMS = rtorrent

rtorrent_LDADD = @PTHREAD_LIBS@ $(top_builddir)/librtorrent.a
rtorrent_SOURCES = main.cc

AM_CPPFLAGS = -I$(srcdir) -I$(top_srcdir) -I$(top_srcdir)/libtorrent -I$(top_srcdir)/libtorrent/src -I$(top_srcdir)/libtorrent/src/torrent '-DPACKAGE_DATADIR="$(pkgdatadir)"'
