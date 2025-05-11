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
	helpers/mock_compare.h \
	helpers/mock_function.cc \
	helpers/mock_function.h \
	helpers/progress_listener.cc \
	helpers/progress_listener.h \
	helpers/protectors.cc \
	helpers/protectors.h \
	helpers/test_fixture.cc \
	helpers/test_fixture.h \
	helpers/test_main_thread.cc \
	helpers/test_main_thread.h \
	helpers/test_thread.cc \
	helpers/test_thread.h \
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
	rpc/test_parse_options.h

rtorrent_Test_Src_SOURCES = $(rtorrent_Test_Common) \
	src/test_command_dynamic.cc \
	src/test_command_dynamic.h

rtorrent_Test_Rpc_CXXFLAGS = $(CPPUNIT_CFLAGS)
rtorrent_Test_Rpc_LDFLAGS = $(CPPUNIT_LIBS) -ldl
rtorrent_Test_Src_CXXFLAGS = $(CPPUNIT_CFLAGS)
rtorrent_Test_Src_LDFLAGS = $(CPPUNIT_LIBS) -ldl

AM_CPPFLAGS = -I$(srcdir) -I$(top_srcdir) -I$(top_srcdir)/src
