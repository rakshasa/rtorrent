#include "config.h"

#include "download.h"

#include <sigc++/bind.h>
#include <sigc++/signal.h>

namespace core {

void
Download::set_download(torrent::Download d) {
  m_download = d;

  m_connTrackerSucceded = m_download.signal_tracker_succeded(sigc::bind(sigc::mem_fun(*this, &Download::receive_tracker_msg), ""));
  m_connTrackerFailed = m_download.signal_tracker_failed(sigc::mem_fun(*this, &Download::receive_tracker_msg));
}

void
Download::release_download() {
  m_connTrackerSucceded.disconnect();
  m_connTrackerFailed.disconnect();
}

void
Download::receive_tracker_msg(std::string msg) {
  m_trackerMsg = msg;
}

}
