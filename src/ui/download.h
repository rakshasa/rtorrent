#ifndef RTORRENT_UI_DOWNLOAD_H
#define RTORRENT_UI_DOWNLOAD_H

#include <list>
#include <torrent/peer.h>
#include <sigc++/connection.h>

#include "core/download_list.h"

namespace display {
  class WindowTitle;
  class WindowPeerList;
  class WindowStatusbar;
}

namespace ui {

class Control;

class Download {
public:
  typedef display::WindowPeerList      WPeerList;
  typedef display::WindowTitle         WTitle;
  typedef display::WindowStatusbar     WStatus;
  typedef core::DownloadList::iterator DPtr;
  typedef std::list<torrent::Peer>     PList;

  // We own 'window'.
  Download(DPtr d, Control* c);
  ~Download();

  WPeerList&       get_window()   { return *m_window; }
  input::Bindings& get_bindings() { return *m_bindings; }

  void             activate();
  void             disable();

private:
  void             receive_next();
  void             receive_prev();

  void             receive_peer_connected(torrent::Peer p);
  void             receive_peer_disconnected(torrent::Peer p);

  void             mark_dirty();

  DPtr             m_download;
  PList            m_peers;
  PList::iterator  m_focus;

  WPeerList*       m_window;
  WTitle*          m_title;
  WStatus*         m_status;

  Control*         m_control;
  input::Bindings* m_bindings;

  sigc::connection m_connPeerConnected;
  sigc::connection m_connPeerDisconnected;
};

}

#endif
