#ifndef RTORRENT_CORE_HASH_QUEUE_H
#define RTORRENT_CORE_HASH_QUEUE_H

#include <list>
#include <sigc++/slot.h>
#include <sigc++/connection.h>

namespace core {

class Download;

class HashQueueNode;

class HashQueue : private std::list<HashQueueNode*> {
public:
  typedef std::list<HashQueueNode*> Base;
  typedef sigc::slot0<void>         Slot;

  using Base::iterator;
  using Base::const_iterator;
  using Base::reverse_iterator;
  using Base::const_reverse_iterator;

  using Base::begin;
  using Base::end;
  using Base::rbegin;
  using Base::rend;

  using Base::front;
  using Base::back;

  using Base::empty;
  using Base::size;

  // Should it be safe to try inserting already present/checked downloads?
  void insert(Download* d, Slot s);

  // It's safe to try to remove downloads not in the queue.
  void remove(Download* d);

private:
  void receive_hash_done(Base::iterator itr);

  void fill_queue();
};

class HashQueueNode {
public:
  HashQueueNode(Download* d, HashQueue::Slot s) : m_download(d), m_slot(s) {}
  ~HashQueueNode()                                    { m_connection.disconnect(); }

  Download*        get_download()                     { return m_download; }
  HashQueue::Slot  get_slot()                         { return m_slot; }

  void             set_connection(sigc::connection c) { m_connection = c; }

private:
  Download*        m_download;
  HashQueue::Slot  m_slot;
  sigc::connection m_connection;
};

}

#endif
