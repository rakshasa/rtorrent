#ifndef RTORRENT_DISPLAY_PEER_LIST_H
#define RTORRENT_DISPLAY_PEER_LIST_H

#include <list>
#include <torrent/peer.h>

#include "window.h"

namespace core {
  class Download;
}

namespace display {

class WindowPeerList : public Window {
public:
  typedef std::list<torrent::Peer> PList;

  WindowPeerList(core::Download* d, PList* l, PList::iterator* f);

  virtual void     redraw();

private:
  int              done_percentage(torrent::Peer& p);

  core::Download*  m_download;

  PList*           m_list;
  PList::iterator* m_focus;
};

}

#endif
