#ifndef RTORRENT_DISPLAY_PEER_LIST_H
#define RTORRENT_DISPLAY_PEER_LIST_H

#include <list>
#include <sigc++/slot.h>
#include <torrent/peer.h>

#include "window.h"

namespace display {

class WindowPeerList : public Window {
public:
  typedef std::list<torrent::Peer> PList;
  typedef sigc::slot0<uint32_t>    SlotChunksTotal;

  WindowPeerList(PList* l);

  PList&          get_list()                           { return *m_list; }

  PList::iterator get_focus()                          { return m_focus; }
  void            set_focus(PList::iterator itr)       { m_focus = itr; }

  void            slot_chunks_total(SlotChunksTotal s) { m_slotChunksTotal = s; }

  virtual void    redraw();

private:
  PList*          m_list;
  PList::iterator m_focus;

  SlotChunksTotal m_slotChunksTotal;
};

}

#endif
