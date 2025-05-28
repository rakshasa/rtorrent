#include "config.h"

#include "window_download_statusbar.h"

#include <torrent/rate.h>
#include <torrent/data/transfer_list.h>
#include <torrent/peer/connection_list.h>
#include <torrent/peer/peer_list.h>

#include "canvas.h"
#include "globals.h"
#include "utils.h"
#include "core/download.h"

namespace display {

WindowDownloadStatusbar::WindowDownloadStatusbar(core::Download* d) :
  Window(new Canvas, 0, 0, 3, extent_full, extent_static),
  m_download(d) {
}

void
WindowDownloadStatusbar::redraw() {
  if (m_canvas->daemon())
    return;

  schedule_update();

  m_canvas->erase();

  std::string buffer(m_canvas->width(), ' ');
  char* last = buffer.data() + m_canvas->width() - 2;

  print_download_info_full(buffer.data(), last, m_download);
  m_canvas->print(0, 0, "%s", buffer.data());

  snprintf(buffer.data(), last - buffer.data(), "Peers: %i(%i) Min/Max: %i/%i Slots: U:%i/%i D:%i/%i U/I/C/A: %i/%i/%i/%i Unchoked: %u/%u Failed: %i",
           (int)m_download->download()->connection_list()->size(),
           (int)m_download->download()->peer_list()->available_list_size(),
           (int)m_download->download()->connection_list()->min_size(),
           (int)m_download->download()->connection_list()->max_size(),
           (int)m_download->download()->uploads_min(),
           (int)m_download->download()->uploads_max(),
           (int)m_download->download()->downloads_min(),
           (int)m_download->download()->downloads_max(),
           (int)m_download->download()->peers_currently_unchoked(),
           (int)m_download->download()->peers_currently_interested(),
           (int)m_download->download()->peers_complete(),
           (int)m_download->download()->peers_accounted(),
           (int)m_download->info()->upload_unchoked(),
           (int)m_download->info()->download_unchoked(),
           (int)m_download->download()->transfer_list()->failed_count());

  m_canvas->print(0, 1, "%s", buffer.data());

  print_download_status(buffer.data(), last, m_download);
  m_canvas->print(0, 2, "[%c:%i] %s",
                  m_download->tracker_controller().has_active_trackers() ? 'C' : ' ',
                  (int)(m_download->download()->tracker_controller().seconds_to_next_timeout()),
                  buffer.data());
}

}
