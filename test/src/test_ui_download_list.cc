#include "config.h"

#include "test/src/test_ui_download_list.h"

#include "ui/download_list.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestUiDownloadList);

void
TestUiDownloadList::test_filter_pattern() {
  CPPUNIT_ASSERT_EQUAL(std::string(""), ui::create_filter_pattern(""));
  CPPUNIT_ASSERT_EQUAL(std::string(".*linux.*"), ui::create_filter_pattern("linux"));
  CPPUNIT_ASSERT_EQUAL(std::string("^linux.*"), ui::create_filter_pattern("^linux"));
  CPPUNIT_ASSERT_EQUAL(std::string(".*linux$"), ui::create_filter_pattern("linux$"));
  CPPUNIT_ASSERT_EQUAL(std::string(".*linux.*"), ui::create_filter_pattern("LiNuX"));
}

void
TestUiDownloadList::test_filter_command() {
  torrent::Object command = ui::create_filter_command(".*linux.*");

  CPPUNIT_ASSERT(command.is_dict_key());
  CPPUNIT_ASSERT_EQUAL(std::string("match"), command.as_dict_key());

  const torrent::Object::list_type& args = command.as_dict_obj().as_list();

  CPPUNIT_ASSERT_EQUAL((size_t)2, args.size());
  CPPUNIT_ASSERT(args.front().is_dict_key());
  CPPUNIT_ASSERT_EQUAL(std::string("d.name"), args.front().as_dict_key());
  CPPUNIT_ASSERT(args.back().is_string());
  CPPUNIT_ASSERT_EQUAL(std::string(".*linux.*"), args.back().as_string());
}

// A pattern that closes the 'match' argument early and appends a second
// command must stay a single inert string argument.
void
TestUiDownloadList::test_filter_command_does_not_inject() {
  const std::string pattern = ui::create_filter_pattern("zzz},$d.custom1.set={pwned");

  CPPUNIT_ASSERT_EQUAL(std::string(".*zzz},$d.custom1.set={pwned.*"), pattern);

  torrent::Object command = ui::create_filter_command(pattern);

  CPPUNIT_ASSERT(command.is_dict_key());
  CPPUNIT_ASSERT_EQUAL(std::string("match"), command.as_dict_key());

  const torrent::Object::list_type& args = command.as_dict_obj().as_list();

  CPPUNIT_ASSERT_EQUAL((size_t)2, args.size());
  CPPUNIT_ASSERT(args.back().is_string());
  CPPUNIT_ASSERT_EQUAL(pattern, args.back().as_string());
}
