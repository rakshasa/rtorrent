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
	helpers/test_utils.h \
	helpers/utils.h

rtorrent_Test_Rpc_SOURCES = $(rtorrent_Test_Common) \
	rpc/test_command.cc \
	rpc/test_command.h \
	rpc/test_command_map.cc \
	rpc/test_command_map.h \
	rpc/test_jsonrpc.cc \
	rpc/test_jsonrpc.h \
	rpc/test_xmlrpc.cc \
	rpc/test_xmlrpc.h \
	rpc/test_command_slot.cc \
	rpc/test_command_slot.h \
	rpc/test_object_storage.cc \
	rpc/test_object_storage.h \
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
