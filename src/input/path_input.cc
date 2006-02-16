// rTorrent - BitTorrent client
// Copyright (C) 2005-2006, Jari Sundell
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
// Contact:  Jari Sundell <jaris@ifi.uio.no>
//
//           Skomakerveien 33
//           3185 Skoppum, NORWAY

#include "config.h"

#include <functional>
#include <rak/algorithm.h>
#include <rak/file_stat.h>
#include <rak/path.h>

#include "path_input.h"

namespace input {

PathInput::PathInput() :
  m_showNext(false) {
}

bool
PathInput::pressed(int key) {
  if (key != '\t') {
    m_showNext = false;
    return TextInput::pressed(key);

  } else if (m_showNext) {
    m_signalShowNext.emit();

  } else {
    receive_do_complete();
  }

  return true;
}

struct _transform_filename {
  _transform_filename(const std::string& base) : m_base(base) {}

  void operator () (std::string& filename) {
    rak::file_stat fs;

    if (!fs.update(rak::path_expand(m_base + filename)))
      return;

    else if (fs.is_directory())
      filename += '/';
  }

  const std::string& m_base;
};

void
PathInput::receive_do_complete() {
  size_type dirEnd = find_last_delim();

  utils::Directory dir(dirEnd != 0 ? str().substr(0, dirEnd) : "./");
  
  if (!dir.update() || dir.empty()) {
    mark_dirty();

    return;
  }

  std::for_each(dir.begin(), dir.end(), _transform_filename(str().substr(0, dirEnd)));

  Range r = find_incomplete(dir, str().substr(dirEnd, get_pos()));

  if (r.first == r.second)
    return; // Show some nice colors here.

  std::string base = rak::make_base(r.first, r.second);

  // Clear the path after the cursor to make this code cleaner. It's
  // not really nessesary to add the complexity just because someone
  // might start tab-completeing in the middle of a path.
  str().resize(dirEnd);
  str().insert(dirEnd, base);

  set_pos(dirEnd + base.size());

  mark_dirty();

  // Only emit if there are more than one option.
  m_showNext = ++utils::Directory::iterator(r.first) != r.second;

  if (m_showNext)
    m_signalShowRange.emit(r.first, r.second);
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

PathInput::Range
PathInput::find_incomplete(utils::Directory& d, const std::string& f) {
  Range r;

  r.first  = std::find_if(d.begin(), d.end(), std::bind2nd(rak::compare_base<std::string>(), f));
  r.second = std::find_if(r.first, d.end(), std::not1(std::bind2nd(rak::compare_base<std::string>(), f)));

  return r;
}

}
