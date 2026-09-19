#include "config.h"

#include "utils/directory.h"

#include <algorithm>
#include <cstdlib>
#include <dirent.h>
#include <fcntl.h>
#include <functional>
#include <sys/stat.h>
#include <torrent/exceptions.h>

#include "globals.h"

namespace utils {

namespace {

uint8_t
entry_type_from_mode(mode_t mode) {
  if (S_ISREG(mode))
    return DT_REG;

  if (S_ISDIR(mode))
    return DT_DIR;

  if (S_ISLNK(mode))
    return DT_LNK;

  return DT_UNKNOWN;
}

} // namespace

// Keep this?
bool
Directory::is_valid() const {
  if (m_path.empty())
    return false;

  DIR* d = opendir(expand_path(m_path).c_str());

  if (d != NULL)
    closedir(d);

  return d;
}

bool
Directory::update(int flags) {
  if (m_path.empty())
    throw torrent::input_error("Directory::update() tried to open an empty path.");

  DIR* d = opendir(expand_path(m_path).c_str());

  if (d == NULL)
    return false;

  struct dirent* entry;

  while ((entry = readdir(d)) != NULL) {
    if ((flags & update_hide_dot) && entry->d_name[0] == '.')
      continue;

    iterator itr = base_type::insert(end(), value_type());

#ifdef __sun__
    itr->s_fileno = entry->d_ino;
    itr->s_reclen = 0;
    itr->s_type   = DT_UNKNOWN;
#else
    itr->s_fileno = entry->d_fileno;
    itr->s_reclen = entry->d_reclen;
    itr->s_type   = entry->d_type;
#endif

    if (itr->s_type == DT_UNKNOWN) {
      struct stat st;

      if (fstatat(dirfd(d), entry->d_name, &st, AT_SYMLINK_NOFOLLOW) == 0)
        itr->s_type = entry_type_from_mode(st.st_mode);
    }

#ifdef DIRENT_NAMLEN_EXISTS_FOOBAR
    itr->s_name   = std::string(entry->d_name, entry->d_name + entry->d_namlen);
#else
    itr->s_name   = std::string(entry->d_name);
#endif
  }

  closedir(d);

  if (flags & update_sort)
    std::sort(begin(), end());

  return true;
}

}
