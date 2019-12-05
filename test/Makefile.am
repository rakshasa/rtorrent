TESTS = rtorrentTest

check_PROGRAMS = $(TESTS)
rtorrentTest_LDADD = ../src/libsub_root.a

rtorrentTest_SOURCES = \
	rpc/command_test.cc \
	rpc/command_test.h \
	rpc/command_map_test.cc \
	rpc/command_map_test.h \
	rpc/command_slot_test.cc \
	rpc/command_slot_test.h \
	rpc/object_storage_test.cc \
	rpc/object_storage_test.h \
	rpc/test_parse_options.cc \
	rpc/test_parse_options.h \
	src/command_dynamic_test.cc \
	src/command_dynamic_test.h \
	main.cc

rtorrentTest_CXXFLAGS = $(CPPUNIT_CFLAGS)
rtorrentTest_LDFLAGS = $(CPPUNIT_LIBS)  -ldl

AM_CPPFLAGS = -I$(srcdir) -I$(top_srcdir) -I$(top_srcdir)/src
