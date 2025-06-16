#include "config.h"

#include <cmath>
#include <stdexcept>
#include <rak/string_manip.h>
#include <torrent/data/block_list.h>
#include <torrent/data/transfer_list.h>

#include "core/download.h"

#include "window_download_transfer_list.h"

namespace display {

WindowDownloadTransferList::WindowDownloadTransferList(core::Download* d, [[maybe_unused]] unsigned int *focus) :
  Window(new Canvas, 0, 0, 0, extent_full, extent_full),
  m_download(d) {
}

void
WindowDownloadTransferList::redraw() {
  // TODO: Make this depend on tracker signal.
  schedule_update();
  m_canvas->erase();

  if (m_canvas->height() < 3 || m_canvas->width() < 18)
    return;

  const torrent::TransferList* transfers = m_download->download()->transfer_list();

  m_canvas->print(2, 0, "Transfer list: [Size %i]", transfers->size());

  torrent::TransferList::const_iterator itr = transfers->begin();

  // This is just for testing and the layout and included information
  // is just something i threw in there, someone really should
  // prettify this. (This is a very subtle hint)

  for (unsigned int y = 1; y < m_canvas->height() && itr != transfers->end(); ++y, ++itr) {
    m_canvas->print(0, y, "%5u [P: %u F: %u]", (*itr)->index(), (*itr)->priority(), (*itr)->failed());

    // Handle window size.
    for (const auto& bItr : **itr) {
      if (m_canvas->get_x() >= m_canvas->width() - 1) {
        if (++y >= m_canvas->height())
          break;

        m_canvas->move(17, y);
      }

      char id;
      chtype attr = A_NORMAL;

      if (bItr.is_finished()) {
        attr = A_REVERSE;
        id   = key_id(bItr.leader()->const_peer_info());

      } else if (bItr.is_transfering()) {
        attr = A_BOLD;
        id   = key_id(bItr.leader()->const_peer_info());

      } else if (bItr.queued()->size() >= 1) {
        id = std::tolower(key_id(bItr.queued()->back()->const_peer_info()));

      } else {
        id = '.';
      }

      if (bItr.size_all() > 1)
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
  uint32_t current_time = torrent::this_thread::cached_seconds().count();
  uint32_t oldest_time = current_time;

  assigned_vector::iterator oldestItr = m_assigned.begin();

  for (assigned_vector::iterator itr = m_assigned.begin(), last = m_assigned.end(); itr != last; ++itr) {
    if (itr->m_key == key) {
      itr->m_last = torrent::this_thread::cached_seconds().count();
      return itr->m_id;
    }

    if (itr->m_last < oldest_time) {
      oldest_time = itr->m_last;
      oldestItr = itr;
    }
  }

  if (oldestItr == m_assigned.end() || current_time - oldest_time <= 60) {
    // We didn't find any previously used id's to take over.

    // Return 'f' when we run out of characters.
    if (m_assigned.size() >= ('Z' - 'A'))
      return 'Z';

    char id = 'A' + m_assigned.size();

    m_assigned.push_back(assigned_type(key, current_time, id));
    return id;

  } else {
    return oldestItr->m_id;
  }
}

}
