#ifndef RTORRENT_UI_DOWNLOAD_H
#define RTORRENT_UI_DOWNLOAD_H

#include <list>
#include <torrent/peer/peer.h>

#include "display/manager.h"
#include "utils/list_focus.h"

#include "element_base.h"

namespace display {
  class WindowDownloadStatusbar;
}

namespace core {
  class Download;
}

namespace ui {

class Download : public ElementBase {
public:
  typedef display::WindowDownloadStatusbar WDownloadStatus;

  typedef std::list<torrent::Peer>         PList;

  typedef enum {
    DISPLAY_MENU,
    DISPLAY_PEER_LIST,
    DISPLAY_INFO,
    DISPLAY_FILE_LIST,
    DISPLAY_TRACKER_LIST,
    DISPLAY_CHUNKS_SEEN,
    DISPLAY_TRANSFER_LIST,
    DISPLAY_MAX_SIZE
  } Display;

  Download(core::Download* d);
  ~Download();

  void                activate(display::Frame* frame, bool focus = true);
  void                disable();

  void                activate_display(Display d, bool focusDisplay);

  void                activate_display_focus(Display d) { activate_display(d, true); }
  void                activate_display_menu(Display d)  { activate_display(d, false); }

  void                receive_next_priority();
  void                receive_prev_priority();

  void                adjust_up_throttle(int throttle);
  void                adjust_down_throttle(int throttle);

  display::Window*    window()   { return NULL; }
  core::Download*     download() { return m_download; };

private:
  Download(const Download&);
  void operator = (const Download&);

  inline ElementBase* create_menu();
  inline ElementBase* create_info();

  void                receive_min_uploads(int t);
  void                receive_max_uploads(int t);
  void                receive_min_downloads(int t);
  void                receive_max_downloads(int t);
  void                receive_min_peers(int t);
  void                receive_max_peers(int t);

  void                bind_keys();

  core::Download*     m_download;

  Display             m_state{DISPLAY_MAX_SIZE};
  ElementBase*        m_uiArray[DISPLAY_MAX_SIZE];

  bool                m_focusDisplay{};

  std::unique_ptr<WDownloadStatus> m_windowDownloadStatus;
};

}

#endif
