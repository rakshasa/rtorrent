#include "config.h"

#include "test/src/test_download_list.h"

#include <cstdio>
#include <torrent/download.h>
#include <torrent/download_info.h>
#include <torrent/hash_string.h>
#include <torrent/object.h>
#include <torrent/torrent.h>

#include "control.h"
#include "core/download.h"
#include "globals.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestDownloadList);

static std::string
insert_download(core::DownloadList* list) {
  torrent::Object info = torrent::Object::create_map();
  info.insert_key("name", std::string("test_download_list"));
  info.insert_key("length", int64_t{16});
  info.insert_key("piece length", int64_t{262144});
  info.insert_key("pieces", std::string(20, char(0)));

  auto* object = new torrent::Object(torrent::Object::create_map());
  object->insert_key("info", info);

  auto download = torrent::download_add(object, 0);
  list->insert(new core::Download(download));

  char buffer[41];

  for (unsigned int i = 0; i < torrent::HashString::size_data; i++)
    snprintf(buffer + i * 2, 3, "%02x", static_cast<unsigned char>(download.info()->hash()[i]));

  return std::string(buffer, 40);
}

void
TestDownloadList::setUp() {
  torrent::initialize_main_thread();
  torrent::initialize();

  if (control == nullptr)
    control = new Control;

  m_hex = insert_download(&m_list);
}

void
TestDownloadList::tearDown() {
  m_list.clear();

  torrent::cleanup();
}

void
TestDownloadList::test_find_hex() {
  CPPUNIT_ASSERT(m_list.find_hex(m_hex.c_str()) != m_list.end());
}

void
TestDownloadList::test_find_hex_wrong_length() {
  CPPUNIT_ASSERT(m_list.find_hex((m_hex + "f").c_str()) == m_list.end());
  CPPUNIT_ASSERT(m_list.find_hex((m_hex + m_hex).c_str()) == m_list.end());
  CPPUNIT_ASSERT(m_list.find_hex(m_hex.substr(0, 39).c_str()) == m_list.end());
}
