#ifndef RTORRENT_CORE_HTTP_QUEUE_H
#define RTORRENT_CORE_HTTP_QUEUE_H

#include <list>
#include <iosfwd>
#include <sigc++/signal.h>

namespace core {

class CurlGet;

class HttpQueue : private std::list<CurlGet*> {
public:
  typedef std::list<CurlGet*>           Base;
  typedef sigc::signal1<void, CurlGet*> SignalHttp;
  typedef sigc::slot0<CurlGet*>         SlotFactory;

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

  // Note that any slots connected to the CurlGet signals must be
  // pushed in front of the erase slot added by HttpQueue::insert.
  iterator    insert(const std::string& url);
  void        erase(iterator itr);

  void        clear();

  void        slot_factory(SlotFactory s) { m_slotFactory = s; }

  SignalHttp& signal_insert()             { return m_signalInsert; }
  SignalHttp& signal_erase()              { return m_signalErase; }

private:
  SlotFactory m_slotFactory;
  SignalHttp  m_signalInsert;
  SignalHttp  m_signalErase;
};

}

#endif
