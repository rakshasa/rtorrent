SUBDIRS = \
	core \
	display \
	input \
	rpc \
	ui \
	utils

bin_PROGRAMS = rtorrent

rtorrent_LDADD = \
	$(top_srcdir)/src/ui/libsub_ui.a \
	$(top_srcdir)/src/core/libsub_core.a \
	$(top_srcdir)/src/display/libsub_display.a \
	$(top_srcdir)/src/input/libsub_input.a \
	$(top_srcdir)/src/rpc/libsub_rpc.a \
	$(top_srcdir)/src/utils/libsub_utils.a

rtorrent_SOURCES = \
	command_download.cc \
	command_events.cc \
	command_file.cc \
	command_helpers.cc \
	command_helpers.h \
	command_local.cc \
	command_network.cc \
	command_tracker.cc \
	command_ui.cc \
	control.cc \
	control.h \
	globals.cc \
	globals.h \
	main.cc \
	option_parser.cc \
	option_parser.h \
	signal_handler.cc \
	signal_handler.h
