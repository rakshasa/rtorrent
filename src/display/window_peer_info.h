#ifndef RTORRENT_DISPLAY_PEER_INFO_H
#define RTORRENT_DISPLAY_PEER_INFO_H

#include <list>
#include <torrent/peer.h>

#include "window.h"

namespace core {
  class Download;
}

namespace display {

class WindowPeerInfo : public Window {
public:
  typedef std::list<torrent::Peer> PList;

  WindowPeerInfo(core::Download* d, PList* l, PList::iterator* f);

  virtual void     redraw();

private:
  core::Download*  m_download;

  PList*           m_list;
  PList::iterator* m_focus;
};

}

#endif
