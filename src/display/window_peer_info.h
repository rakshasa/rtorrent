#ifndef RTORRENT_DISPLAY_PEER_INFO_H
#define RTORRENT_DISPLAY_PEER_INFO_H

#include <list>
#include <sigc++/slot.h>
#include <torrent/peer.h>

#include "window.h"

namespace display {

class WindowPeerInfo : public Window {
public:
  typedef std::list<torrent::Peer> PList;
  typedef sigc::slot0<uint32_t>    SlotChunksTotal;

  WindowPeerInfo(PList* l, PList::iterator* f);

  void             slot_chunks_total(SlotChunksTotal s) { m_slotChunksTotal = s; }

  virtual void     redraw();

private:
  PList*           m_list;
  PList::iterator* m_focus;

  SlotChunksTotal  m_slotChunksTotal;
};

}

#endif
