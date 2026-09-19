#include "config.h"

#include "test/src/test_session_commit.h"

#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

#include "session/download_storer.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestSessionCommit);

namespace {

const char* entry_name = "0123456789ABCDEF0123456789ABCDEF01234567.torrent";

std::string
read_file(const std::string& path) {
  std::ifstream     file(path.c_str());
  std::stringstream buffer;

  buffer << file.rdbuf();
  return buffer.str();
}

void
remove_directory(const std::string& path) {
  DIR* d = ::opendir(path.c_str());

  if (d == NULL)
    return;

  struct dirent* entry;

  while ((entry = ::readdir(d)) != NULL) {
    if (entry->d_name[0] == '.' && (entry->d_name[1] == '\0' || (entry->d_name[1] == '.' && entry->d_name[2] == '\0')))
      continue;

    ::unlink((path + "/" + entry->d_name).c_str());
  }

  ::closedir(d);
  ::rmdir(path.c_str());
}

} // namespace

void
TestSessionCommit::setUp() {
  test_fixture::setUp();

  char temp_dir[] = "/tmp/rtorrent_test_commit_XXXXXX";

  CPPUNIT_ASSERT(mkdtemp(temp_dir) != nullptr);
  m_session_dir = temp_dir;
}

void
TestSessionCommit::tearDown() {
  remove_directory(m_session_dir);

  test_fixture::tearDown();
}

void
TestSessionCommit::commit_and_verify(bool use_fsyncdisk) {
  auto path = m_session_dir + "/" + entry_name;

  std::stringstream torrent_stream("torrent-data");
  std::stringstream rtorrent_stream("rtorrent-data");
  std::stringstream libtorrent_stream("libtorrent-data");

  session::DownloadStorer::save_and_move_streams(path, use_fsyncdisk, &torrent_stream, &rtorrent_stream, &libtorrent_stream);

  CPPUNIT_ASSERT_EQUAL(std::string("torrent-data"), read_file(path));
  CPPUNIT_ASSERT_EQUAL(std::string("rtorrent-data"), read_file(path + ".rtorrent"));
  CPPUNIT_ASSERT_EQUAL(std::string("libtorrent-data"), read_file(path + ".libtorrent_resume"));

  struct stat st;
  CPPUNIT_ASSERT(::stat((path + ".new").c_str(), &st) == -1);
}

void
TestSessionCommit::test_commit_publishes_all_three_files() {
  commit_and_verify(false);
}

void
TestSessionCommit::test_commit_with_fsync_publishes_all_three_files() {
  commit_and_verify(true);
}
