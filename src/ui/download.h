#ifndef RTORRENT_UI_DOWNLOAD_H
#define RTORRENT_UI_DOWNLOAD_H

#include <list>
#include <torrent/peer.h>
#include <sigc++/connection.h>

namespace display {
  class WindowTitle;
  class WindowPeerInfo;
  class WindowPeerList;
  class WindowStatusbar;
}

namespace core {
  class Download;
}

namespace ui {

class Control;

class Download {
public:
  typedef enum {
    DISPLAY_NONE,
    DISPLAY_MAIN,
    DISPLAY_PEER
  } Display;

  typedef display::Window              Window;
  typedef display::WindowPeerInfo      WPeerInfo;
  typedef display::WindowPeerList      WPeerList;
  typedef display::WindowTitle         WTitle;
  typedef display::WindowStatusbar     WStatus;

  typedef core::Download*              DPtr;
  typedef std::list<torrent::Peer>     PList;
  typedef display::Manager::iterator   MItr;

  // We own 'window'.
  Download(DPtr d, Control* c);
  ~Download();

  //WPeerList&       get_window()   { return *m_window; }
  input::Bindings& get_bindings() { return *m_bindings; }

  void             activate();
  void             disable();

  void             activate_display(Display d);
  void             disable_display();

private:
  Download(const Download&);
  void operator = (const Download&);

  void             receive_next();
  void             receive_prev();

  void             receive_peer_connected(torrent::Peer p);
  void             receive_peer_disconnected(torrent::Peer p);

  void             receive_throttle(int t);
  void             receive_change(Display d);

  void             bind_keys(input::Bindings* b);

  void             mark_dirty();

  DPtr             m_download;
  PList            m_peers;
  PList::iterator  m_focus;

  Display          m_state;
  MItr             m_window;

  WTitle*          m_title;
  WStatus*         m_status;

  Control*         m_control;
  input::Bindings* m_bindings;

  sigc::connection m_connPeerConnected;
  sigc::connection m_connPeerDisconnected;
};

}

#endif
