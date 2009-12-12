SUBDIRS = \
	core \
	display \
	input \
	rpc \
	ui \
	utils

bin_PROGRAMS = rtorrent

rtorrent_LDADD = \
	ui/libsub_ui.a \
	core/libsub_core.a \
	display/libsub_display.a \
	input/libsub_input.a \
	rpc/libsub_rpc.a \
	utils/libsub_utils.a

rtorrent_SOURCES = \
	command_download.cc \
	command_dynamic.cc \
	command_events.cc \
	command_file.cc \
	command_helpers.cc \
	command_helpers.h \
	command_local.cc \
	command_network.cc \
	command_object.cc \
	command_peer.cc \
	command_tracker.cc \
	command_scheduler.cc \
	command_ui.cc \
	control.cc \
	control.h \
	globals.cc \
	globals.h \
	main.cc \
	option_parser.cc \
	option_parser.h \
	signal_handler.cc \
	signal_handler.h \
	thread_base.cc \
	thread_base.h \
	thread_worker.cc \
	thread_worker.h

INCLUDES = -I$(srcdir) -I$(top_srcdir)
