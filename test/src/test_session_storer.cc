#include "config.h"

#include "test/src/test_session_storer.h"

#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

#include "session/download_storer.h"
#include "utils/directory.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestSessionStorer);

namespace {

const char* entry_name = "0123456789ABCDEF0123456789ABCDEF01234567.torrent";
const char* link_name  = "FEDCBA9876543210FEDCBA9876543210FEDCBA98.torrent";

void
write_file(const std::string& path, const std::string& content) {
  std::ofstream file(path.c_str());

  file << content;
  file.close();

  CPPUNIT_ASSERT(file.good());
}

std::string
read_file(const std::string& path) {
  std::ifstream    file(path.c_str());
  std::stringstream buffer;

  buffer << file.rdbuf();
  return buffer.str();
}

void
save_session_files(const std::string& path) {
  std::stringstream torrent_stream("torrent-data");
  std::stringstream rtorrent_stream("rtorrent-data");
  std::stringstream libtorrent_stream("libtorrent-data");

  session::DownloadStorer::save_and_move_streams(path, false, &torrent_stream, &rtorrent_stream, &libtorrent_stream);
}

unsigned int
permissions_of(const std::string& path) {
  struct stat st;

  CPPUNIT_ASSERT_EQUAL(0, ::stat(path.c_str(), &st));
  return st.st_mode & 07777;
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
TestSessionStorer::setUp() {
  test_fixture::setUp();

  char temp_dir[] = "/tmp/rtorrent_test_session_XXXXXX";

  CPPUNIT_ASSERT(mkdtemp(temp_dir) != nullptr);

  m_temp_dir    = temp_dir;
  m_session_dir = m_temp_dir + "/session";

  CPPUNIT_ASSERT_EQUAL(0, ::mkdir(m_session_dir.c_str(), 0755));
}

void
TestSessionStorer::tearDown() {
  remove_directory(m_session_dir);
  remove_directory(m_temp_dir);

  test_fixture::tearDown();
}

// A symlink planted where the next temporary session file will be written must
// not redirect the write to the file it points at.
void
TestSessionStorer::test_temp_file_symlink_is_not_followed() {
  auto outside = m_temp_dir + "/outside.txt";
  auto path    = m_session_dir + "/" + entry_name;

  write_file(outside, "original");
  CPPUNIT_ASSERT_EQUAL(0, ::symlink(outside.c_str(), (path + ".new").c_str()));

  save_session_files(path);

  CPPUNIT_ASSERT_EQUAL(std::string("original"), read_file(outside));
  CPPUNIT_ASSERT_EQUAL(std::string("torrent-data"), read_file(path));
}

// Session files carry tracker announce urls, so they must not be readable by
// other users regardless of the umask rtorrent was started with.
void
TestSessionStorer::test_saved_files_are_owner_only() {
  auto path       = m_session_dir + "/" + entry_name;
  auto prev_umask = ::umask(0);

  save_session_files(path);
  ::umask(prev_umask);

  CPPUNIT_ASSERT_EQUAL(0600u, permissions_of(path));
  CPPUNIT_ASSERT_EQUAL(0600u, permissions_of(path + ".rtorrent"));
  CPPUNIT_ASSERT_EQUAL(0600u, permissions_of(path + ".libtorrent_resume"));
}

// Entries listed for loading must report their real type so that a symlink in
// the session directory is skipped instead of loaded.
void
TestSessionStorer::test_symlinked_entry_is_not_a_file() {
  write_file(m_temp_dir + "/outside.txt", "d0:e");
  write_file(m_session_dir + "/" + entry_name, "d0:e");

  CPPUNIT_ASSERT_EQUAL(0, ::symlink((m_temp_dir + "/outside.txt").c_str(), (m_session_dir + "/" + link_name).c_str()));

  auto entries = session::DownloadStorer::get_formated_entries(m_session_dir + "/");

  CPPUNIT_ASSERT_EQUAL(size_t{2}, size_t{entries.size()});

  for (const auto& entry : entries) {
    if (entry.s_name == entry_name)
      CPPUNIT_ASSERT(entry.is_file());
    else
      CPPUNIT_ASSERT(!entry.is_file());
  }
}
