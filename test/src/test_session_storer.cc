#include "config.h"

#include "test/src/test_session_storer.h"

#include <cerrno>
#include <cstdlib>
#include <dirent.h>
#include <dlfcn.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <torrent/exceptions.h>

#include "session/download_storer.h"
#include "utils/directory.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestSessionStorer);

namespace {

thread_local int mode_error        = 0;
thread_local int mode_calls        = 0;
thread_local int mode_failure_call = 0;

struct ModeError {
  ModeError(int error, int call = 0) {
    mode_error        = error;
    mode_calls        = 0;
    mode_failure_call = call;
  }
  ~ModeError() { mode_error = 0; }
};

} // namespace

extern "C" int
fchmod(int fd, mode_t mode) noexcept(noexcept(::fchmod(fd, mode))) {
  ++mode_calls;
  if (mode_error != 0 && (mode_failure_call == 0 || mode_calls == mode_failure_call)) {
    errno = mode_error;
    return -1;
  }

  static auto real_fchmod = reinterpret_cast<int (*)(int, mode_t)>(dlsym(RTLD_NEXT, "fchmod"));
  if (real_fchmod == nullptr)
    std::abort();
  return real_fchmod(fd, mode);
}

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
save_session_files(const std::string& path, mode_t file_mode = 0600) {
  std::stringstream torrent_stream("torrent-data");
  std::stringstream rtorrent_stream("rtorrent-data");
  std::stringstream libtorrent_stream("libtorrent-data");

  CPPUNIT_ASSERT_EQUAL(0, session::DownloadStorer::save_and_move_streams(path, false, file_mode, &torrent_stream, &rtorrent_stream, &libtorrent_stream));
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

void
TestSessionStorer::test_saved_files_use_configured_mode() {
  for (mode_t mode : {0660, 0640}) {
    auto path       = m_session_dir + "/" + entry_name;
    auto prev_umask = ::umask(0);

    save_session_files(path, mode);
    ::umask(prev_umask);

    CPPUNIT_ASSERT_EQUAL((unsigned int)mode, permissions_of(path));
    CPPUNIT_ASSERT_EQUAL((unsigned int)mode, permissions_of(path + ".rtorrent"));
    CPPUNIT_ASSERT_EQUAL((unsigned int)mode, permissions_of(path + ".libtorrent_resume"));
  }
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

void
TestSessionStorer::test_mode_failure_preserves_payload() {
  auto              path = m_session_dir + "/" + entry_name;
  std::stringstream torrent_stream("torrent-data");
  std::stringstream rtorrent_stream("rtorrent-data");
  std::stringstream libtorrent_stream("libtorrent-data");
  ModeError         failure(EPERM);

  CPPUNIT_ASSERT_EQUAL(EPERM, session::DownloadStorer::save_and_move_streams(path, true, 0660, &torrent_stream, &rtorrent_stream, &libtorrent_stream));

  for (auto suffix : {"", ".rtorrent", ".libtorrent_resume"})
    CPPUNIT_ASSERT_EQUAL(0600u, permissions_of(path + suffix));

  CPPUNIT_ASSERT_EQUAL(std::string("torrent-data"), read_file(path));
  CPPUNIT_ASSERT_EQUAL(std::string("rtorrent-data"), read_file(path + ".rtorrent"));
  CPPUNIT_ASSERT_EQUAL(std::string("libtorrent-data"), read_file(path + ".libtorrent_resume"));
}

void
TestSessionStorer::test_mode_failure_still_refuses_storage_errors() {
  auto              path = m_session_dir + "/" + entry_name;
  std::stringstream stream("new-data");
  ModeError         failure(EPERM);

  CPPUNIT_ASSERT_EQUAL(0, ::mkdir((path + ".rtorrent").c_str(), 0700));
  CPPUNIT_ASSERT_THROW(session::DownloadStorer::save_and_move_streams(path, true, 0660, &stream, &stream, &stream), torrent::storage_error);
  CPPUNIT_ASSERT_EQUAL(0, ::rmdir((path + ".rtorrent").c_str()));
}

void
TestSessionStorer::test_mode_failure_on_each_stream() {
  auto              path = m_session_dir + "/" + entry_name;
  std::stringstream stream("new-data");
  const char*       suffixes[] = {"", ".libtorrent_resume", ".rtorrent"};

  for (int call : {1, 2, 3}) {
    ModeError failure(ENOSYS, call);
    CPPUNIT_ASSERT_EQUAL(ENOSYS, session::DownloadStorer::save_and_move_streams(path, false, 0640, &stream, &stream, &stream));
    CPPUNIT_ASSERT_EQUAL(3, mode_calls);
    for (int index = 0; index < 3; ++index) {
      CPPUNIT_ASSERT_EQUAL(index + 1 == call ? 0600u : 0640u, permissions_of(path + suffixes[index]));
      CPPUNIT_ASSERT_EQUAL(std::string("new-data"), read_file(path + suffixes[index]));
    }
  }

  ModeError failure(EPERM);
  CPPUNIT_ASSERT_EQUAL(0, session::DownloadStorer::save_and_move_streams(path, false, 0, nullptr, &stream, &stream));
  CPPUNIT_ASSERT_EQUAL(0, mode_calls);
}
