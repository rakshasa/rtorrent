#include "config.h"

#include "canvas.h"
#include "window_download_statusbar.h"

#include "core/download.h"

namespace display {

WindowDownloadStatusbar::WindowDownloadStatusbar(core::Download* d) :
  Window(new Canvas, false, 3),
  m_download(d) {
}

void
WindowDownloadStatusbar::redraw() {
  if (Timer::cache() - m_lastDraw < 1000000)
    return;

  m_lastDraw = Timer::cache();

  m_canvas->erase();

  if (m_download->get_download().get_chunks_done() != m_download->get_download().get_chunks_total() || !m_download->get_download().is_open())
    m_canvas->print(0, 0, "Torrent: %.1f / %.1f MiB Rate:%5.1f /%5.1f KiB Uploaded: %.1f MiB",
		    (double)m_download->get_download().get_bytes_done() / (double)(1 << 20),
		    (double)m_download->get_download().get_bytes_total() / (double)(1 << 20),
		    (double)m_download->get_download().get_rate_up() / 1024.0,
		    (double)m_download->get_download().get_rate_down() / 1024.0,
		    (double)m_download->get_download().get_bytes_up() / (double)(1 << 20));
 
  else
    m_canvas->print(0, 0, "Torrent: Done %.1f MiB Rate:%5.1f /%5.1f KiB Uploaded: %.1f MiB",
		    (double)m_download->get_download().get_bytes_total() / (double)(1 << 20),
		    (double)m_download->get_download().get_rate_up() / 1024.0,
		    (double)m_download->get_download().get_rate_down() / 1024.0,
		    (double)m_download->get_download().get_bytes_up() / (double)(1 << 20));
    
  m_canvas->print(0, 1, "Peers: %i(%i) Min/Max: %i/%i Uploads: %i",
		  (int)m_download->get_download().get_peers_connected(),
		  (int)m_download->get_download().get_peers_not_connected(),
		  (int)m_download->get_download().get_peers_min(),
		  (int)m_download->get_download().get_peers_max(),
		  (int)m_download->get_download().get_uploads_max());

  m_canvas->print(0, 2, "Tracker: [%c:%i] %s",
		  m_download->get_download().is_tracker_busy() ? 'C' : ' ',
		  (int)(m_download->get_download().get_tracker_timeout() / 1000000),
		  m_download->get_tracker_msg().c_str());
}

}
