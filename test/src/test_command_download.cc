#include "config.h"

#include "test/src/test_command_download.h"

#include <torrent/download.h>
#include <torrent/object.h>
#include <torrent/torrent.h>
#include <torrent/utils/string_manip.h>

#include "control.h"
#include "core/download.h"
#include "globals.h"
#include "rpc/parse_commands.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestCommandDownload);

void initialize_command_download();

namespace {

// Valid UTF-8 with a carriage return and a C0 control character.
const std::string control_name = "cr\r ctl\x01 \xc3\xa9.bin";

core::Download*
insert_download(core::DownloadList* list, const std::string& name) {
  torrent::Object info = torrent::Object::create_map();
  info.insert_key("name", name);
  info.insert_key("length", int64_t{16});
  info.insert_key("piece length", int64_t{262144});
  info.insert_key("pieces", std::string(20, char(0)));

  auto* object = new torrent::Object(torrent::Object::create_map());
  object->insert_key("info", info);

  auto* download = new core::Download(torrent::download_add(object, 0));
  list->insert(download);

  return download;
}

torrent::Object
call_d(const char* key, core::Download* download) {
  return rpc::commands.call_command_d(key, download, torrent::Object());
}

}

void
TestCommandDownload::setUp() {
  // torrent::download_add() opens a socket through the mocked fd functions.
  mock_redirect_defaults();

  torrent::initialize_main_thread();
  torrent::initialize();

  if (control == nullptr)
    control = new Control;

  if (!rpc::commands.has("d.name"))
    initialize_command_download();
}

void
TestCommandDownload::tearDown() {
  m_list.clear();

  torrent::cleanup();
}

void
TestCommandDownload::test_name_or_base64() {
  auto* plain    = insert_download(&m_list, "plain.bin");
  auto* download = insert_download(&m_list, control_name);

  auto plain_result = call_d("d.name.or_base64", plain);
  CPPUNIT_ASSERT_EQUAL(std::string("plain.bin"), plain_result.as_string());
  CPPUNIT_ASSERT_EQUAL(uint32_t{0}, plain_result.flags());

  auto result = call_d("d.name.or_base64", download);
  CPPUNIT_ASSERT_EQUAL(torrent::utils::transform_to_base64(control_name), result.as_string());
  CPPUNIT_ASSERT_EQUAL(torrent::Object::flag_base64 | torrent::Object::flag_as_binary, result.flags());

  CPPUNIT_ASSERT_EQUAL(control_name, call_d("d.name", download).as_string());
  CPPUNIT_ASSERT_EQUAL(uint32_t{0}, call_d("d.name", download).flags());
}

void
TestCommandDownload::test_name_or_as_binary() {
  auto* plain    = insert_download(&m_list, "plain.bin");
  auto* download = insert_download(&m_list, control_name);

  auto plain_result = call_d("d.name.or_as_binary", plain);
  CPPUNIT_ASSERT_EQUAL(std::string("plain.bin"), plain_result.as_string());
  CPPUNIT_ASSERT_EQUAL(uint32_t{0}, plain_result.flags());

  auto result = call_d("d.name.or_as_binary", download);
  CPPUNIT_ASSERT_EQUAL(control_name, result.as_string());
  CPPUNIT_ASSERT_EQUAL(torrent::Object::flag_as_binary, result.flags());
}
