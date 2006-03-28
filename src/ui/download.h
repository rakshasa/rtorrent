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

#ifndef RTORRENT_UI_DOWNLOAD_H
#define RTORRENT_UI_DOWNLOAD_H

#include <list>
#include <torrent/peer.h>
#include <sigc++/connection.h>

#include "display/manager.h"
#include "utils/list_focus.h"

class Control;

namespace display {
  class WindowTitle;
  class WindowStatusbar;
  class WindowDownloadStatusbar;
}

namespace core {
  class Download;
}

namespace ui {

class ElementBase;

class Download {
public:
  typedef display::WindowTitle             WTitle;
  typedef display::WindowDownloadStatusbar WDownloadStatus;

  typedef core::Download*                  DPtr;
  typedef std::list<torrent::Peer>         PList;
  typedef display::Manager::iterator       MItr;

  typedef enum {
    DISPLAY_PEER_LIST,
    DISPLAY_PEER_INFO,
    DISPLAY_FILE_LIST,
    DISPLAY_TRACKER_LIST,
    DISPLAY_CHUNKS_SEEN,
    DISPLAY_MAX_SIZE
  } Display;

  Download(DPtr d, Control* c);
  ~Download();

  input::Bindings&    get_bindings() { return *m_bindings; }

  void                activate();
  void                disable();

  void                activate_display(Display d);
  void                disable_display();

  void                receive_next_priority();
  void                receive_prev_priority();

private:
  Download(const Download&);
  void operator = (const Download&);

  void                receive_next();
  void                receive_prev();

  void                receive_disconnect_peer();

  void                receive_peer_connected(torrent::Peer p);
  void                receive_peer_disconnected(torrent::Peer p);

  void                receive_max_uploads(int t);
  void                receive_min_peers(int t);
  void                receive_max_peers(int t);
  void                receive_change(Display d);

  void                receive_snub_peer();
  void                receive_manual_request(bool force);

  void                bind_keys();

  void                mark_dirty();

  DPtr                m_download;
  PList               m_peers;
  PList::iterator     m_focus;

  Display             m_state;

  ElementBase*        m_uiArray[DISPLAY_MAX_SIZE];

  WTitle*             m_windowTitle;
  WDownloadStatus*    m_windowDownloadStatus;

  MItr                m_window;

  Control*            m_control;
  input::Bindings*    m_bindings;

  sigc::connection    m_connPeerConnected;
  sigc::connection    m_connPeerDisconnected;
};

}

#endif
