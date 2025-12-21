// DownloadStore handles the saving and listing of session torrents.

#include "config.h"

#include <fstream>
#include <stdio.h>
#include <fcntl.h>
#include <sstream>
#include <unistd.h>
#include <rak/error_number.h>
#include <rak/path.h>
#include <rak/string_manip.h>
#include <torrent/utils/resume.h>
#include <torrent/object.h>
#include <torrent/exceptions.h>
#include <torrent/torrent.h>
#include <torrent/rate.h>
#include <torrent/object_stream.h>

#include "download.h"
#include "download_store.h"
#include "rpc/parse_commands.h"
#include "session/session_manager.h"
#include "utils/directory.h"

namespace core {

// This also needs to check that it isn't a directory.
bool
not_correct_format(const utils::directory_entry& entry) {
  return !DownloadStore::is_correct_format(entry.s_name);
}

utils::Directory
DownloadStore::get_formated_entries() {
  if (!session_thread::manager()->is_used())
    return utils::Directory();

  utils::Directory d(session_thread::manager()->path());

  if (!d.update(utils::Directory::update_hide_dot))
    throw torrent::storage_error("core::DownloadStore::update() could not open session directory: " + session_thread::manager()->path());

  d.erase(std::remove_if(d.begin(), d.end(), [&](const utils::directory_entry& entry) { return not_correct_format(entry); }), d.end());

  return d;
}

bool
DownloadStore::is_correct_format(const std::string& f) {
  if (f.size() != 48 || f.substr(40) != ".torrent")
    return false;

  for (std::string::const_iterator itr = f.begin(); itr != f.end() - 8; ++itr)
    if (!(*itr >= '0' && *itr <= '9') &&
        !(*itr >= 'A' && *itr <= 'F'))
      return false;

  return true;
}

}
