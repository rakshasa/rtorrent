SUBDIRS = \
	display

bin_PROGRAMS = rtorrent

rtorrent_LDFLAGS = -version-info $(RTORRENT_INTERFACE_VERSION_INFO)
rtorrent_LDADD = \
	$(top_srcdir)/src/display/libsub_display.a

rtorrent_SOURCES = \
	rtorrent.cc
