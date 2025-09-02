// rTorrent - BitTorrent client
// Copyright (C) 2005-2011, Jari Sundell
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// In addition, as a special exception, the copyright holders give
// permission to link the code of portions of this program with the
// OpenSSL library under certain conditions as described in each
// individual source file, and distribute linked combinations
// including the two.
//
// You must obey the GNU General Public License in all respects for
// all of the code used other than OpenSSL.  If you modify file(s)
// with this exception, you may extend this exception to your version
// of the file(s), but you are not obligated to do so.  If you do not
// wish to do so, delete this exception statement from your version.
// If you delete this exception statement from all source files in the
// program, then also delete it here.
//
// Contact:  Jari Sundell <sundell.software@gmail.com>


#include "config.h"

#include <functional>
#include <rak/algorithm.h>
#include <rak/path.h>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <torrent/utils/log.h>

#include "path_input.h"

namespace input {

bool
PathInput::pressed(int key) {
  // Consider binding tab in m_bindings instead.

  if (key != '\t') {
    m_showNext = false;
    return TextInput::pressed(key);

  } else if (m_showNext) {
    for (auto& itr : m_signal_show_next)
      itr();

  } else {
    receive_do_complete();
  }

  return true;
}

void
PathInput::receive_do_complete() {
  lt_log_print(torrent::LOG_UI_EVENTS, "path_input: received completion");

  size_type dirEnd = find_last_delim();

  utils::Directory dir(dirEnd != 0 ? str().substr(0, dirEnd) : "./");
  
  if (!dir.update(utils::Directory::update_sort | utils::Directory::update_hide_dot) || dir.empty()) {
    mark_dirty();

    return;
  }

  for (auto& entry : dir) {
#ifdef __sun__
    if (entry.s_type & S_IFDIR)
#else
    if (entry.s_type == DT_DIR)
#endif
      entry.s_name += '/';
  }

  range_type r = find_incomplete(dir, str().substr(dirEnd, get_pos()));

  if (r.first == r.second)
    return; // Show some nice colors here.

  std::string base = rak::make_base<std::string>(r.first, r.second, std::mem_fn(&utils::directory_entry::s_name));

  // Clear the path after the cursor to make this code cleaner. It's
  // not really nessesary to add the complexity just because someone
  // might start tab-completeing in the middle of a path.
  str().resize(dirEnd);
  str().insert(dirEnd, base);

  set_pos(dirEnd + base.size());

  mark_dirty();

  // Only emit if there are more than one option.
  m_showNext = ++utils::Directory::iterator(r.first) != r.second;

  if (m_showNext) {
    lt_log_print(torrent::LOG_UI_EVENTS, "path_input: show next page");

    for (auto& itr : m_signal_show_range)
      itr(r.first, r.second);
  }
}

PathInput::size_type
PathInput::find_last_delim() {
  size_type r = str().rfind('/', get_pos());

  if (r == npos)
    return 0;
  else if (r == size())
    return r;
  else
    return r + 1;
}

inline bool
find_complete_compare(const utils::directory_entry& complete, const std::string& base) {
  return complete.s_name.compare(0, base.size(), base);
}

inline bool
find_complete_not_compare(const utils::directory_entry& complete, const std::string& base) {
  return !complete.s_name.compare(0, base.size(), base);
}

PathInput::range_type
PathInput::find_incomplete(utils::Directory& d, const std::string& f) {
  range_type r;

  r.first  = std::find_if(d.begin(), d.end(), [f](const utils::directory_entry& de) { 
      return find_complete_not_compare(de, f); 
  });
  r.second = std::find_if(r.first,   d.end(), [f](const utils::directory_entry& de) { 
      return find_complete_compare(de, f); 
  });

  return r;
}

}
