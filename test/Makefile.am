TESTS = \
	rtorrent_Test_Rpc \
	rtorrent_Test_Src

check_PROGRAMS = $(TESTS)

rtorrent_Test_LDADD = \
	../src/libsub_root.a

rtorrent_Test_Rpc_LDADD = $(rtorrent_Test_LDADD)
rtorrent_Test_Src_LDADD = $(rtorrent_Test_LDADD)

rtorrent_Test_Common = \
	main.cc \
	\
	helpers/assert.h \
	helpers/progress_listener.cc \
	helpers/progress_listener.h \
	helpers/protectors.cc \
	helpers/protectors.h \
	helpers/utils.h

rtorrent_Test_Rpc_SOURCES = $(rtorrent_Test_Common) \
	rpc/command_test.cc \
	rpc/command_test.h \
	rpc/command_map_test.cc \
	rpc/command_map_test.h \
	rpc/jsonrpc_test.cc \
	rpc/jsonrpc_test.h \
	rpc/xmlrpc_test.cc \
	rpc/xmlrpc_test.h \
	rpc/command_slot_test.cc \
	rpc/command_slot_test.h \
	rpc/object_storage_test.cc \
	rpc/object_storage_test.h \
	rpc/test_parse_options.cc \
	rpc/test_parse_options.h \
	src/command_dynamic_test.cc \
	src/command_dynamic_test.h

rtorrent_Test_Src_SOURCES = $(rtorrent_Test_Common) \
	src/command_dynamic_test.cc \
	src/command_dynamic_test.h

rtorrent_Test_Rpc_CXXFLAGS = $(CPPUNIT_CFLAGS)
rtorrent_Test_Rpc_LDFLAGS = $(CPPUNIT_LIBS) -ldl
rtorrent_Test_Src_CXXFLAGS = $(CPPUNIT_CFLAGS)
rtorrent_Test_Src_LDFLAGS = $(CPPUNIT_LIBS) -ldl

AM_CPPFLAGS = -I$(srcdir) -I$(top_srcdir) -I$(top_srcdir)/src
