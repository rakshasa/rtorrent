SUBDIRS = \
	display \
	input

bin_PROGRAMS = rtorrent

rtorrent_LDADD = \
	$(top_srcdir)/src/display/libsub_display.a \
	$(top_srcdir)/src/input/libsub_input.a

rtorrent_SOURCES = \
	main.cc
