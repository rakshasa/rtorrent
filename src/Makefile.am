SUBDIRS = \
	core \
	display \
	input \
	rpc \
	ui \
	utils

noinst_LIBRARIES = libsub_root.a

libsub_root_a_SOURCES = \
	command_download.cc \
	command_dynamic.cc \
	command_events.cc \
	command_file.cc \
	command_ip.cc \
	command_helpers.cc \
	command_helpers.h \
	command_groups.cc \
	command_local.cc \
	command_network.cc \
	command_peer.cc \
	command_throttle.cc \
	command_tracker.cc \
	command_scheduler.cc \
	command_ui.cc \
	control.cc \
	control.h \
	globals.cc \
	globals.h \
	option_parser.cc \
	option_parser.h \
	signal_handler.cc \
	signal_handler.h \
	thread_base.cc \
	thread_base.h \
	thread_worker.cc \
	thread_worker.h

bin_PROGRAMS = rtorrent

rtorrent_LDADD = \
	libsub_root.a \
	ui/libsub_ui.a \
	core/libsub_core.a \
	display/libsub_display.a \
	input/libsub_input.a \
	rpc/libsub_rpc.a \
	utils/libsub_utils.a \
	@PTHREAD_LIBS@

rtorrent_SOURCES = \
	main.cc

AM_CPPFLAGS = -I$(srcdir) -I$(top_srcdir)
