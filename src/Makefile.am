SUBDIRS = \
	core \
	display \
	input \
	ui

bin_PROGRAMS = rtorrent

rtorrent_LDADD = \
	$(top_srcdir)/src/ui/libsub_ui.a \
	$(top_srcdir)/src/core/libsub_core.a \
	$(top_srcdir)/src/display/libsub_display.a \
	$(top_srcdir)/src/input/libsub_input.a

rtorrent_SOURCES = \
	functional.h \
	main.cc \
	option_parser.cc \
	option_parser.h \
	parse.cc \
	parse.h \
	signal_handler.cc \
	signal_handler.h \
	timer.h
