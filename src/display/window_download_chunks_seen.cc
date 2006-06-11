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
#include <torrent/bitfield.h>
#include <torrent/transfer_list.h>
#include <torrent/block_list.h>

#include "core/download.h"

#include "window_download_chunks_seen.h"

namespace display {

WindowDownloadChunksSeen::WindowDownloadChunksSeen(core::Download* d, unsigned int *focus) :
  Window(new Canvas, true),
  m_download(d),
  m_focus(focus) {
}

void
WindowDownloadChunksSeen::redraw() {
  // TODO: Make this depend on tracker signal.
  m_slotSchedule(this, (cachedTime + rak::timer::from_seconds(10)).round_seconds());
  m_canvas->erase();

  if (m_canvas->get_height() < 3 || m_canvas->get_width() < 18)
    return;

  m_canvas->print(2, 0, "Chunks seen: [C/A/D %i/%i/%.2f]",
                  (int)m_download->download()->peers_complete(),
                  (int)m_download->download()->peers_accounted(),
                  std::floor(m_download->distributed_copies() * 100.0f) / 100.0f);

  const uint8_t* seen = m_download->download()->chunks_seen();

  if (seen == NULL) {
    m_canvas->print(2, 2, "Not available.");
    return;
  }

  if (!m_download->is_done()) { 
    m_canvas->print(36, 0, "X downloaded    missing    downloading");
    m_canvas->print_char(50, 0, 'X' | A_BOLD);
    m_canvas->print_char(61, 0, 'X' | A_REVERSE);
  }

  *m_focus = std::min(*m_focus, max_focus());

  const uint8_t* chunk = seen + *m_focus * chunks_per_row();
  const uint8_t* last = seen + m_download->download()->chunks_total();

  const torrent::Bitfield* bitfield = m_download->download()->bitfield();
  const torrent::TransferList* transfers = m_download->download()->transfer_list();
  std::vector<uint32_t> transferChunks(transfers->size(), 0);

  std::transform(transfers->begin(), transfers->end(), transferChunks.begin(), std::mem_fun(&torrent::BlockList::index));
  std::sort(transferChunks.begin(), transferChunks.end());

  std::vector<uint32_t>::const_iterator itrTransfer = transferChunks.begin();

  for (int y = 1; y < m_canvas->get_height() && chunk < last; ++y) {
    m_canvas->print(0, y, "%5d ", (int)(chunk - seen));

    while (chunk < last) {
      chtype attr;

      if (bitfield->get(chunk - seen)) {
        attr = A_NORMAL;
      } else if (itrTransfer != transferChunks.end() && (uint32_t)(chunk - seen) == *itrTransfer) {
        attr = A_REVERSE;
        itrTransfer++;
      } else {
        attr = A_BOLD;
      }

      m_canvas->print_char(attr | rak::value_to_hexchar<0>(std::min<uint8_t>(*chunk, 0xF)));
      chunk++;

      if ((chunk - seen) % 10 == 0) {
        if (m_canvas->get_x() + 12 > m_canvas->get_width())
          break;

        m_canvas->print_char(' ');
      }
    }
  }
}

unsigned int
WindowDownloadChunksSeen::rows() const {
  if (m_canvas->get_width() < 18)
    return 0;

  return (m_download->download()->chunks_total() + chunks_per_row() - 1) / chunks_per_row();
}

}
