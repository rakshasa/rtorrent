#include "config.h"

#include "test/src/test_command_path.h"

#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>
#include <torrent/exceptions.h>
#include <torrent/torrent.h>

#include "control.h"
#include "globals.h"
#include "rpc/parse_commands.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestCommandPath);

void initialize_command_local();

void
TestCommandPath::setUp() {
  char temp_dir[] = "/tmp/rtorrent_test_path_XXXXXX";

  CPPUNIT_ASSERT(mkdtemp(temp_dir) != nullptr);

  // The temporary directory itself may sit behind a symlink, as /tmp does on
  // macOS, so resolve it up front to keep the expected values exact.
  m_temp_dir = resolve_path(temp_dir);

  CPPUNIT_ASSERT(!m_temp_dir.empty());
  CPPUNIT_ASSERT_EQUAL(0, mkdir((m_temp_dir + "/data").c_str(), 0755));
  CPPUNIT_ASSERT_EQUAL(0, symlink((m_temp_dir + "/data").c_str(), (m_temp_dir + "/link").c_str()));
}

void
TestCommandPath::tearDown() {
  unlink((m_temp_dir + "/link").c_str());
  rmdir((m_temp_dir + "/data").c_str());
  rmdir(m_temp_dir.c_str());
}

void
TestCommandPath::test_resolves_symlink() {
  CPPUNIT_ASSERT_EQUAL(m_temp_dir + "/data", resolve_path(m_temp_dir + "/link"));

  // A path that lies below a symlinked directory is resolved as well.
  CPPUNIT_ASSERT_EQUAL(0, mkdir((m_temp_dir + "/data/below").c_str(), 0755));
  CPPUNIT_ASSERT_EQUAL(m_temp_dir + "/data/below", resolve_path(m_temp_dir + "/link/below"));
  rmdir((m_temp_dir + "/data/below").c_str());
}

void
TestCommandPath::test_removes_relative_components() {
  CPPUNIT_ASSERT_EQUAL(m_temp_dir, resolve_path(m_temp_dir + "/data/.."));
  CPPUNIT_ASSERT_EQUAL(m_temp_dir + "/data", resolve_path(m_temp_dir + "/./data"));
  CPPUNIT_ASSERT_EQUAL(m_temp_dir + "/data", resolve_path(m_temp_dir + "/data/"));

  // Trailing slashes and duplicated separators collapse.
  CPPUNIT_ASSERT_EQUAL(m_temp_dir + "/data", resolve_path(m_temp_dir + "//data//"));
}

void
TestCommandPath::test_expands_tilde() {
  const char* home = std::getenv("HOME");

  if (home == nullptr || *home == '\0')
    return;

  CPPUNIT_ASSERT_EQUAL(resolve_path(home), resolve_path("~"));
  CPPUNIT_ASSERT_THROW(resolve_path("~root/somewhere"), torrent::input_error);
}

void
TestCommandPath::test_missing_path_throws() {
  CPPUNIT_ASSERT_THROW(resolve_path_or_throw(""), torrent::input_error);
  CPPUNIT_ASSERT_THROW(resolve_path_or_throw(m_temp_dir + "/does_not_exist"), torrent::input_error);
  CPPUNIT_ASSERT_EQUAL(m_temp_dir + "/data", resolve_path_or_throw(m_temp_dir + "/link"));
}

void
TestCommandPath::test_missing_path_is_empty() {
  CPPUNIT_ASSERT_EQUAL(std::string(), resolve_path(""));
  CPPUNIT_ASSERT_EQUAL(std::string(), resolve_path(m_temp_dir + "/does_not_exist"));
  CPPUNIT_ASSERT_EQUAL(std::string(), resolve_path(m_temp_dir + "/does_not_exist/below"));

  // A dangling symlink does not name an existing path either.
  CPPUNIT_ASSERT_EQUAL(0, symlink((m_temp_dir + "/gone").c_str(), (m_temp_dir + "/dangling").c_str()));
  CPPUNIT_ASSERT_EQUAL(std::string(), resolve_path(m_temp_dir + "/dangling"));
  unlink((m_temp_dir + "/dangling").c_str());
}

void
TestCommandPath::test_commands() {
  torrent::initialize_main_thread();
  torrent::initialize();

  if (control == nullptr)
    control = new Control;

  if (!rpc::commands.has("directory.default.realpath.or_empty"))
    initialize_command_local();

  rpc::commands.call_command("directory.default.set", m_temp_dir + "/link");

  CPPUNIT_ASSERT_EQUAL(m_temp_dir + "/link", rpc::commands.call_command("directory.default", torrent::Object()).as_string());
  CPPUNIT_ASSERT_EQUAL(m_temp_dir + "/data", rpc::commands.call_command("directory.default.realpath.or_empty", torrent::Object()).as_string());
  CPPUNIT_ASSERT_EQUAL(m_temp_dir + "/data", rpc::commands.call_command("directory.default.realpath.or_throw", torrent::Object()).as_string());

  // A directory that has not been created yet resolves to nothing, and the
  // or_throw variant reports it instead.
  rpc::commands.call_command("directory.default.set", m_temp_dir + "/missing");

  CPPUNIT_ASSERT_EQUAL(std::string(), rpc::commands.call_command("directory.default.realpath.or_empty", torrent::Object()).as_string());
  CPPUNIT_ASSERT_THROW(rpc::commands.call_command("directory.default.realpath.or_throw", torrent::Object()), torrent::input_error);

  torrent::cleanup();
}
