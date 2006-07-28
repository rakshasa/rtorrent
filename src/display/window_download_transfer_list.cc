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

#include <cmath>
#include <stdexcept>
#include <rak/string_manip.h>
#include <torrent/block_list.h>
#include <torrent/transfer_list.h>

#include "core/download.h"

#include "window_download_transfer_list.h"

namespace display {

WindowDownloadTransferList::WindowDownloadTransferList(core::Download* d, unsigned int *focus) :
  Window(new Canvas, flag_width_dynamic | flag_height_dynamic, 0, 0),
  m_download(d),
  m_focus(focus) {
}

void
WindowDownloadTransferList::redraw() {
  // TODO: Make this depend on tracker signal.
  m_slotSchedule(this, (cachedTime + rak::timer::from_seconds(1)).round_seconds());
  m_canvas->erase();

  if (m_canvas->height() < 3 || m_canvas->width() < 18)
    return;

  const torrent::TransferList* transfers = m_download->download()->transfer_list();

  m_canvas->print(2, 0, "Transfer list: [Size %i]", transfers->size());

  torrent::TransferList::const_iterator itr = transfers->begin();

  // This is just for testing and the layout and included information
  // is just something i threw in there, someone really should
  // prettify this. (This is a very subtle hint)

  for (int y = 1; y < m_canvas->height() && itr != transfers->end(); ++y, ++itr) {
    m_canvas->print(0, y, "%5u [P: %u F: %u]", (*itr)->index(), (*itr)->priority(), (*itr)->failed());

    // Handle window size.
    for (torrent::BlockList::const_iterator bItr = (*itr)->begin(), bLast = (*itr)->end(); bItr != bLast; ++bItr) {
      char id;
      chtype attr = A_NORMAL;

      if (bItr->is_finished()) {
        attr = A_REVERSE;
        id = key_id(bItr->leader()->const_peer_info());
        
      } else if (bItr->is_transfering()) {
        attr = A_BOLD;
        id = key_id(bItr->leader()->const_peer_info());

      } else if (bItr->queued()->size() >= 1) {
        id = std::tolower(key_id(bItr->queued()->back()->const_peer_info()));

      } else {
        id = '.';
      }

      if (bItr->size_all() > 1)
        attr |= A_UNDERLINE;

      m_canvas->print_char(attr | id);
    }      
  }
}

unsigned int
WindowDownloadTransferList::rows() const {
  if (m_canvas->width() < 18)
    return 0;

//   return (m_download->download()->chunks_total() + chunks_per_row() - 1) / chunks_per_row();
  return 0;
}

char
WindowDownloadTransferList::key_id(torrent::BlockTransfer::key_type key) {
  uint32_t oldestTime = cachedTime.seconds();
  assigned_vector::iterator oldestItr = m_assigned.begin();

  for (assigned_vector::iterator itr = m_assigned.begin(), last = m_assigned.end(); itr != last; ++itr) {
    if (itr->m_key == key) {
      itr->m_last = cachedTime.seconds();
      return itr->m_id;
    }

    if (itr->m_last < oldestTime) {
      oldestTime = itr->m_last;
      oldestItr = itr;
    }
  }

  if (oldestItr == m_assigned.end() || cachedTime.seconds() - oldestTime <= 60) {
    // We didn't find any previously used id's to take over.

    // Return 'f' when we run out of characters.
    if (m_assigned.size() >= ('Z' - 'A'))
      return 'Z';

    char id = 'A' + m_assigned.size();

    m_assigned.push_back(assigned_type(key, cachedTime.seconds(), id));
    return id;

  } else {
    return oldestItr->m_id;
  }
}

}
