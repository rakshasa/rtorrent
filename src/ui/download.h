// rTorrent - BitTorrent client
// Copyright (C) 2005-2011, Jari Sundell
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

  display::Window*    window() { return NULL; }

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

  Display             m_state;
  ElementBase*        m_uiArray[DISPLAY_MAX_SIZE];

  bool                m_focusDisplay;

  WDownloadStatus*    m_windowDownloadStatus;
};

}

#endif
