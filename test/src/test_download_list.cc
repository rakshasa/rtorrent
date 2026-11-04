#include "config.h"

#include "test/src/test_download_list.h"

#include <cstdio>
#include <torrent/download.h>
#include <torrent/download_info.h>
#include <torrent/exceptions.h>
#include <torrent/hash_string.h>
#include <torrent/object.h>
#include <torrent/torrent.h>
#include <vector>

#include "control.h"
#include "core/download.h"
#include "globals.h"
#include "session/thread_session.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestDownloadList);

static core::Download*
new_download(const std::string& name) {
  torrent::Object info = torrent::Object::create_map();
  info.insert_key("name", name);
  info.insert_key("length", int64_t{16});
  info.insert_key("piece length", int64_t{262144});
  info.insert_key("pieces", std::string(20, char(0)));

  auto* object = new torrent::Object(torrent::Object::create_map());
  object->insert_key("info", info);

  return new core::Download(torrent::download_add(object, 0));
}

static core::Download*
insert_download(core::DownloadList* list, const std::string& name) {
  auto download = new_download(name);
  list->insert(download);

  return download;
}

static std::vector<core::Download*>
list_order(const core::DownloadList& list) {
  std::vector<core::Download*> downloads;

  for (const auto& download : list)
    downloads.push_back(download.get());

  return downloads;
}

static std::string
hex_of(core::Download* download) {
  char buffer[41];

  for (unsigned int i = 0; i < torrent::HashString::size_data; i++)
    snprintf(buffer + i * 2, 3, "%02x", static_cast<unsigned char>(download->info()->hash()[i]));

  return std::string(buffer, 40);
}

void
TestDownloadList::setUp() {
  torrent::initialize_main_thread();
  torrent::initialize();
  session::ThreadSession::create_thread();

  if (control == nullptr)
    control = new Control;

  m_hex = hex_of(insert_download(&m_list, "test_download_list"));
}

void
TestDownloadList::tearDown() {
  m_list.clear();

  session::ThreadSession::destroy_thread();
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

void
TestDownloadList::test_find_hex_after_erase() {
  std::vector<core::Download*> downloads;
  std::vector<std::string>     hexes;

  for (int i = 0; i < 100; i++) {
    downloads.push_back(insert_download(&m_list, "test_download_list_" + std::to_string(i)));
    hexes.push_back(hex_of(downloads.back()));
  }

  for (size_t i = 0; i < downloads.size(); i += 2)
    m_list.erase_ptr(downloads[i]);

  for (size_t i = 0; i < downloads.size(); i++) {
    auto expected = i % 2 == 0 ? nullptr : downloads[i];

    CPPUNIT_ASSERT(m_list.find_hex_ptr(hexes[i].c_str()) == expected);
  }

  CPPUNIT_ASSERT_EQUAL(size_t(51), m_list.size());
}

void
TestDownloadList::test_insertion_order() {
  auto d0 = m_list.find_hex_ptr(m_hex.c_str());
  auto d1 = insert_download(&m_list, "test_download_list_1");
  auto d2 = insert_download(&m_list, "test_download_list_2");
  auto d3 = insert_download(&m_list, "test_download_list_3");

  m_list.erase_ptr(d0);

  CPPUNIT_ASSERT(m_list.erase(m_list.find_hex(hex_of(d2).c_str()))->get() == d3);

  auto d4 = new_download("test_download_list_4");

  CPPUNIT_ASSERT(m_list.insert(d4)->get() == d4);
  CPPUNIT_ASSERT(list_order(m_list) == (std::vector<core::Download*>{d1, d3, d4}));
}

void
TestDownloadList::test_insert_duplicate() {
  auto d0 = m_list.find_hex_ptr(m_hex.c_str());

  CPPUNIT_ASSERT_THROW(m_list.insert(new core::Download(*d0->download())), torrent::internal_error);
}
