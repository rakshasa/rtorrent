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

#include <stdexcept>
#include <rak/algorithm.h>
#include <torrent/file.h>
#include <torrent/file_list.h>
#include <torrent/path.h>

#include "core/download.h"

#include "window_file_list.h"

namespace display {

WindowFileList::WindowFileList(core::Download* d, unsigned int* focus) :
  Window(new Canvas, flag_width_dynamic | flag_height_dynamic, 0, 0),
  m_download(d),
  m_focus(focus) {
}

/*
std::wstring
hack_wstring(const std::string& src) {
  size_t length = ::mbstowcs(NULL, src.c_str(), src.size());

  if (length == (size_t)-1)
    return std::wstring(L"<invalid>");

  std::wstring dest;
  dest.resize(length);
  
  ::mbstowcs(&*dest.begin(), src.c_str(), src.size());

  return dest;
}
*/

void
WindowFileList::redraw() {
  m_slotSchedule(this, (cachedTime + rak::timer::from_seconds(10)).round_seconds());
  m_canvas->erase();

  torrent::FileList fl = m_download->download()->file_list();

  if (fl.size() == 0 || m_canvas->height() < 2)
    return;

  int pos = 0;

  m_canvas->print( 2, pos, "File");
  m_canvas->print(55, pos, "Size");
  m_canvas->print(63, pos, "Pri");
  m_canvas->print(68, pos, "Cmpl");
  m_canvas->print(74, pos, "Encoding");
  m_canvas->print(84, pos, "Chunks");

  ++pos;

  if (*m_focus >= fl.size())
    throw std::logic_error("WindowFileList::redraw() called on an object with a bad focus value");

  Range range = rak::advance_bidirectional<unsigned int>(0, *m_focus, fl.size(), m_canvas->height() - pos);

  while (range.first != range.second) {
    torrent::File e = fl.get(range.first);

    std::string path = e.path_str();

    if (path.length() <= 50)
      path = path + std::string(50 - path.length(), ' ');
    else
      path = path.substr(0, 50);

    std::string priority;

    switch (e.priority()) {
    case torrent::PRIORITY_OFF:
      priority = "off";
      break;

    case torrent::PRIORITY_NORMAL:
      priority = "   ";
      break;

    case torrent::PRIORITY_HIGH:
      priority = "hig";
      break;

    default:
      priority = "BUG";
      break;
    };

    m_canvas->print(0, pos, "%c %s  %6.1f   %s   %3d  %9s",
                    range.first == *m_focus ? '*' : ' ',
                    path.c_str(),
                    (double)e.size_bytes() / (double)(1 << 20),
                    priority.c_str(),
                    done_percentage(e),
                    e.path()->encoding().c_str());

    m_canvas->print(84, pos, "%i - %i %c%c",
                    e.chunk_begin(),
                    e.chunk_begin() != e.chunk_end() ? (e.chunk_end() - 1) : e.chunk_end(),
                    e.is_created() ? 'E' : 'M',
                    e.is_correct_size() ? 'C' : 'W');

    ++range.first;
    ++pos;
  }

}

int
WindowFileList::done_percentage(torrent::File& e) {
  int chunks = e.chunk_end() - e.chunk_begin();

  return chunks ? (e.completed_chunks() * 100) / chunks : 100;
}

}
