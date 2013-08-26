TESTS = rtorrentTest
AUTOMAKE_OPTIONS = subdir-objects

check_PROGRAMS = $(TESTS)
rtorrentTest_LDADD = \
	../src/libsub_root.a \
	../src/ui/libsub_ui.a \
	../src/core/libsub_core.a \
	../src/display/libsub_display.a \
	../src/input/libsub_input.a \
	../src/rpc/libsub_rpc.a \
	../src/utils/libsub_utils.a

rtorrentTest_SOURCES = \
	rpc/command_test.cc \
	rpc/command_test.h \
	rpc/command_map_test.cc \
	rpc/command_map_test.h \
	rpc/command_slot_test.cc \
	rpc/command_slot_test.h \
	rpc/object_storage_test.cc \
	rpc/object_storage_test.h \
	src/command_dynamic_test.cc \
	src/command_dynamic_test.h \
	main.cc

rtorrentTest_CXXFLAGS = $(CPPUNIT_CFLAGS)
rtorrentTest_LDFLAGS = $(CPPUNIT_LIBS)  -ldl

AM_CPPFLAGS = -I$(srcdir) -I$(top_srcdir) -I$(top_srcdir)/src
