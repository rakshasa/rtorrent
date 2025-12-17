// A simple, and not guaranteed atomic, lockfile implementation. It
// saves the hostname and pid in the lock file, which may be accessed
// by Lockfile::locked_by(). If the path is an empty string then no
// lockfile will be created when Lockfile::try_lock() is called, still
// it will set the locked state of the Lockfile instance.

#ifndef RTORRENT_UTILS_LOCKFILE_H
#define RTORRENT_UTILS_LOCKFILE_H

#include <string>
#include <sys/types.h>

namespace utils {

class Lockfile {
public:
  typedef std::pair<std::string, pid_t> process_type;

  bool                is_locked() const                 { return m_locked; }
  bool                is_stale();

  // If the path is empty no lock file will be created, although
  // is_locked() will return true.
  bool                try_lock();
  bool                unlock();

  const std::string&  path() const                      { return m_path; }
  void                set_path(const std::string& path) { m_path = path; }

  std::string         locked_by_as_string() const;
  process_type        locked_by() const;

private:
  std::string         m_path;
  bool                m_locked{};
};

}

#endif
