#include "config.h"

#include "test/src/test_setup.h"

#include <fstream>
#include <string>
#include <unistd.h>
#include <torrent/exceptions.h>

#include "setup.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestSetup);

// Linking setup.o pulls in the help printer, which lives in src/main.cc.
void
print_help() {}

namespace {

class temp_config_file {
public:
  temp_config_file(const std::string& contents) {
    char path[] = "/tmp/rtorrent_test_setup_XXXXXX";

    CPPUNIT_ASSERT(::mkstemp(path) != -1);
    m_path = path;

    std::ofstream file(m_path);
    file << contents << '\n';
  }

  ~temp_config_file() { ::unlink(m_path.c_str()); }

  const std::string& path() const { return m_path; }

private:
  std::string m_path;
};

// The group name must be a valid one, else option_find_string throws before
// the output argument is ever touched.
void
assert_arg_count_error(const std::string& line) {
  temp_config_file file(line);

  try {
    parse_config_file_comments(file.path());
  } catch (torrent::input_error& e) {
    CPPUNIT_ASSERT_EQUAL(std::string("Invalid number of arguments."), std::string(e.what()));
    return;
  }

  CPPUNIT_FAIL("no torrent::input_error thrown for: " + line);
}

} // namespace

void
TestSetup::test_config_comment_log_add_output() {
  temp_config_file file("# do:log.add_output=debug,test_output");

  CPPUNIT_ASSERT_NO_THROW(parse_config_file_comments(file.path()));
}

void
TestSetup::test_config_comment_log_add_output_no_args() {
  assert_arg_count_error("# do:log.add_output=");
}

void
TestSetup::test_config_comment_log_add_output_one_arg() {
  assert_arg_count_error("# do:log.add_output=debug");
}

void
TestSetup::test_config_comment_log_add_output_too_many_args() {
  assert_arg_count_error("# do:log.add_output=debug,test_output,extra");
}
