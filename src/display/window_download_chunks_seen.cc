#include "config.h"

#include <cmath>
#include <stdexcept>
#include <rak/string_manip.h>
#include <torrent/bitfield.h>
#include <torrent/data/block.h>
#include <torrent/data/block_list.h>
#include <torrent/data/transfer_list.h>

#include "core/download.h"

#include "window_download_chunks_seen.h"

namespace display {

WindowDownloadChunksSeen::WindowDownloadChunksSeen(core::Download* d, unsigned int *focus) :
  Window(new Canvas, 0, 0, 0, extent_full, extent_full),
  m_download(d),
  m_focus(focus) {
}

void
WindowDownloadChunksSeen::redraw() {
  // TODO: Make this depend on tracker signal.
  schedule_update(10);
  m_canvas->erase();

  if (m_canvas->height() < 3 || m_canvas->width() < 18)
    return;

  m_canvas->print(2, 0, "Chunks seen: [C/A/D %i/%i/%.2f]",
                  (int)m_download->download()->peers_complete() + m_download->download()->file_list()->is_done(),
                  (int)m_download->download()->peers_accounted(),
                  std::floor(m_download->distributed_copies() * 100.0f) / 100.0f);

  const uint8_t* seen = m_download->download()->chunks_seen();

  if (seen == NULL || m_download->download()->file_list()->bitfield()->empty()) {
    m_canvas->print(2, 2, "Not available.");
    return;
  }

  if (!m_download->is_done()) {
    m_canvas->print(36, 0, "X downloaded    missing    queued    downloading");
    m_canvas->print_char(50, 0, 'X' | A_BOLD);
    m_canvas->print_char(61, 0, 'X' | A_BOLD | A_UNDERLINE);
    m_canvas->print_char(71, 0, 'X' | A_REVERSE);
  }

  *m_focus = std::min(*m_focus, max_focus());

  const uint8_t* chunk = seen + *m_focus * chunks_per_row();
  const uint8_t* last = seen + m_download->download()->file_list()->size_chunks();

  const torrent::Bitfield* bitfield = m_download->download()->file_list()->bitfield();
  const torrent::TransferList* transfers = m_download->download()->transfer_list();
  std::vector<torrent::BlockList*> transferChunks(transfers->begin(), transfers->end());

  std::sort(transferChunks.begin(), transferChunks.end());

  std::vector<torrent::BlockList*>::const_iterator itrTransfer = transferChunks.begin();

  while (itrTransfer != transferChunks.end() && (uint32_t)(chunk - seen) > (*itrTransfer)->index())
    itrTransfer++;

  for (unsigned int y = 1; y < m_canvas->height() && chunk < last; ++y) {
    m_canvas->print(0, y, "%5u ", (int)(chunk - seen));

    while (chunk < last) {
      chtype attr;

      if (bitfield->get(chunk - seen)) {
        attr = A_NORMAL;
      } else if (itrTransfer != transferChunks.end() && (uint32_t)(chunk - seen) == (*itrTransfer)->index()) {
        if (std::any_of((*itrTransfer)->begin(), (*itrTransfer)->end(), std::mem_fn(&torrent::Block::is_transfering)))
          attr = A_REVERSE;
        else
          attr = A_BOLD | A_UNDERLINE;
        itrTransfer++;
      } else {
        attr = A_BOLD;
      }

      m_canvas->print_char(attr | rak::value_to_hexchar<0>(std::min<uint8_t>(*chunk, 0xF)));
      chunk++;

      if ((chunk - seen) % 10 == 0) {
        if (m_canvas->get_x() + 12 > m_canvas->width())
          break;

        m_canvas->print_char(' ');
      }
    }
  }
}

unsigned int
WindowDownloadChunksSeen::rows() const {
  if (m_canvas->width() < 18)
    return 0;

  return (m_download->download()->file_list()->size_chunks() + chunks_per_row() - 1) / chunks_per_row();
}

}
