TESTS = \
	LibTorrent_Test_Torrent_Net \
	LibTorrent_Test_Torrent_Utils \
	LibTorrent_Test_Torrent \
	LibTorrent_Test_Data \
	LibTorrent_Test_Net \
	LibTorrent_Test_Tracker \
	LibTorrent_Test

check_PROGRAMS = $(TESTS)

LT_TEST_DIR = $(top_srcdir)/libtorrent/test

# Override DEFAULT_INCLUDES so we control the -I order.
# Libtorrent paths must come first so its own headers shadow rtorrent's.
DEFAULT_INCLUDES =

AM_CPPFLAGS = \
	-I$(top_srcdir)/libtorrent/test \
	-I$(top_srcdir)/libtorrent \
	-I$(top_srcdir)/libtorrent/src \
	-I$(srcdir) \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src

AM_CXXFLAGS = $(CPPUNIT_CFLAGS) -DLT_CPPUNIT_TESTING

LibTorrent_Test_LDADD = $(top_builddir)/librtorrent.a $(CPPUNIT_LIBS)

LibTorrent_Test_Torrent_Net_LDADD = $(LibTorrent_Test_LDADD)
LibTorrent_Test_Torrent_Utils_LDADD = $(LibTorrent_Test_LDADD)
LibTorrent_Test_Torrent_LDADD = $(LibTorrent_Test_LDADD)
LibTorrent_Test_Data_LDADD = $(LibTorrent_Test_LDADD)
LibTorrent_Test_Net_LDADD = $(LibTorrent_Test_LDADD)
LibTorrent_Test_Tracker_LDADD = $(LibTorrent_Test_LDADD)

LibTorrent_Test_Common = \
	$(LT_TEST_DIR)/main.cc \
	$(LT_TEST_DIR)/helpers/expect_fd.h \
	$(LT_TEST_DIR)/helpers/expect_utils.h \
	$(LT_TEST_DIR)/helpers/mock_compare.h \
	$(LT_TEST_DIR)/helpers/mock_function.cc \
	$(LT_TEST_DIR)/helpers/mock_function.h \
	$(LT_TEST_DIR)/helpers/network.cc \
	$(LT_TEST_DIR)/helpers/network.h \
	$(LT_TEST_DIR)/helpers/progress_listener.cc \
	$(LT_TEST_DIR)/helpers/progress_listener.h \
	$(LT_TEST_DIR)/helpers/protectors.cc \
	$(LT_TEST_DIR)/helpers/protectors.h \
	$(LT_TEST_DIR)/helpers/test_fixture.cc \
	$(LT_TEST_DIR)/helpers/test_fixture.h \
	$(LT_TEST_DIR)/helpers/test_main_thread.cc \
	$(LT_TEST_DIR)/helpers/test_main_thread.h \
	$(LT_TEST_DIR)/helpers/test_thread.cc \
	$(LT_TEST_DIR)/helpers/test_thread.h \
	$(LT_TEST_DIR)/helpers/test_utils.h \
	$(LT_TEST_DIR)/helpers/tracker_test.cc \
	$(LT_TEST_DIR)/helpers/tracker_test.h \
	$(LT_TEST_DIR)/helpers/utils.h

LibTorrent_Test_Torrent_Net_SOURCES = $(LibTorrent_Test_Common) \
	$(LT_TEST_DIR)/torrent/net/test_address_info.cc \
	$(LT_TEST_DIR)/torrent/net/test_address_info.h \
	$(LT_TEST_DIR)/torrent/net/test_fd.cc \
	$(LT_TEST_DIR)/torrent/net/test_fd.h \
	$(LT_TEST_DIR)/torrent/net/test_socket_address.cc \
	$(LT_TEST_DIR)/torrent/net/test_socket_address.h

LibTorrent_Test_Torrent_Utils_SOURCES = $(LibTorrent_Test_Common) \
	$(LT_TEST_DIR)/torrent/utils/test_extents.cc \
	$(LT_TEST_DIR)/torrent/utils/test_extents.h \
	$(LT_TEST_DIR)/torrent/utils/test_log.cc \
	$(LT_TEST_DIR)/torrent/utils/test_log.h \
	$(LT_TEST_DIR)/torrent/utils/test_log_buffer.cc \
	$(LT_TEST_DIR)/torrent/utils/test_log_buffer.h \
	$(LT_TEST_DIR)/torrent/utils/directory_events_test.cc \
	$(LT_TEST_DIR)/torrent/utils/directory_events_test.h \
	$(LT_TEST_DIR)/torrent/utils/test_option_strings.cc \
	$(LT_TEST_DIR)/torrent/utils/test_option_strings.h \
	$(LT_TEST_DIR)/torrent/utils/test_queue_buckets.cc \
	$(LT_TEST_DIR)/torrent/utils/test_queue_buckets.h \
	$(LT_TEST_DIR)/torrent/utils/test_thread_base.cc \
	$(LT_TEST_DIR)/torrent/utils/test_thread_base.h \
	$(LT_TEST_DIR)/torrent/utils/test_uri_parser.cc \
	$(LT_TEST_DIR)/torrent/utils/test_uri_parser.h

LibTorrent_Test_Torrent_SOURCES = $(LibTorrent_Test_Common) \
	$(LT_TEST_DIR)/torrent/object_test.cc \
	$(LT_TEST_DIR)/torrent/object_test.h \
	$(LT_TEST_DIR)/torrent/object_test_utils.cc \
	$(LT_TEST_DIR)/torrent/object_test_utils.h \
	$(LT_TEST_DIR)/torrent/object_static_map_test.cc \
	$(LT_TEST_DIR)/torrent/object_static_map_test.h \
	$(LT_TEST_DIR)/torrent/object_stream_test.cc \
	$(LT_TEST_DIR)/torrent/object_stream_test.h \
	$(LT_TEST_DIR)/torrent/test_tracker_controller.cc \
	$(LT_TEST_DIR)/torrent/test_tracker_controller.h \
	$(LT_TEST_DIR)/torrent/test_tracker_controller_features.cc \
	$(LT_TEST_DIR)/torrent/test_tracker_controller_features.h \
	$(LT_TEST_DIR)/torrent/test_tracker_controller_requesting.cc \
	$(LT_TEST_DIR)/torrent/test_tracker_controller_requesting.h \
	$(LT_TEST_DIR)/torrent/test_tracker_list.cc \
	$(LT_TEST_DIR)/torrent/test_tracker_list.h \
	$(LT_TEST_DIR)/torrent/test_tracker_list_features.cc \
	$(LT_TEST_DIR)/torrent/test_tracker_list_features.h \
	$(LT_TEST_DIR)/torrent/test_tracker_timeout.cc \
	$(LT_TEST_DIR)/torrent/test_tracker_timeout.h

LibTorrent_Test_Data_SOURCES = $(LibTorrent_Test_Common) \
	$(LT_TEST_DIR)/data/test_chunk_list.cc \
	$(LT_TEST_DIR)/data/test_chunk_list.h \
	$(LT_TEST_DIR)/data/test_hash_check_queue.cc \
	$(LT_TEST_DIR)/data/test_hash_check_queue.h \
	$(LT_TEST_DIR)/data/test_hash_queue.cc \
	$(LT_TEST_DIR)/data/test_hash_queue.h

LibTorrent_Test_Net_SOURCES = $(LibTorrent_Test_Common)

LibTorrent_Test_Tracker_SOURCES = $(LibTorrent_Test_Common) \
	$(LT_TEST_DIR)/tracker/test_tracker_http.cc \
	$(LT_TEST_DIR)/tracker/test_tracker_http.h

LibTorrent_Test_SOURCES = $(LibTorrent_Test_Common) \
	$(LT_TEST_DIR)/rak/ranges_test.cc \
	$(LT_TEST_DIR)/rak/ranges_test.h \
	$(LT_TEST_DIR)/protocol/test_request_list.cc \
	$(LT_TEST_DIR)/protocol/test_request_list.h


