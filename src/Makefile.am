SUBDIRS = \
	display \
	engine \
	input

bin_PROGRAMS = rtorrent

rtorrent_LDADD = \
	$(top_srcdir)/src/display/libsub_display.a \
	$(top_srcdir)/src/engine/libsub_engine.a \
	$(top_srcdir)/src/input/libsub_input.a

rtorrent_SOURCES = \
	main.cc \
	signal_handler.cc \
	signal_handler.h
