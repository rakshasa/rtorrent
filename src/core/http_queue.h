#ifndef RTORRENT_CORE_HTTP_QUEUE_H
#define RTORRENT_CORE_HTTP_QUEUE_H

#include <list>
#include <iosfwd>
#include <sigc++/slot.h>

namespace torrent {
  class Http;
}

namespace core {

class HttpQueue : private std::list<torrent::Http*> {
public:
  typedef std::list<torrent::Http*>   Base;
  typedef sigc::slot0<torrent::Http*> SlotFactory;

  using Base::iterator;
  using Base::const_iterator;
  using Base::reverse_iterator;
  using Base::const_reverse_iterator;

  using Base::begin;
  using Base::end;
  using Base::rbegin;
  using Base::rend;

  using Base::empty;
  using Base::size;

  void        insert(const std::string& url);
  void        erase(iterator itr);

  void        clear();

  void        slot_factory(SlotFactory s) { m_slotFactory = s; }

private:
  void        receive_done(iterator itr);
  void        receive_failed(iterator itr, std::string msg);

  SlotFactory m_slotFactory;
};

}

#endif
