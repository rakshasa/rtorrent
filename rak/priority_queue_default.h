#ifndef RAK_PRIORITY_QUEUE_DEFAULT_H
#define RAK_PRIORITY_QUEUE_DEFAULT_H

#include <cassert>
#include <functional>
#include <rak/priority_queue.h>
#include <rak/timer.h>

#include "torrent/exceptions.h"

namespace rak {

class priority_item {
public:
  typedef std::function<void (void)> slot_void;

  priority_item() {}
  ~priority_item() {
    assert(!is_queued() && "priority_item::~priority_item() called on a queued item.");

    m_time = timer();
    m_slot = slot_void();
  }

  bool                is_valid() const              { return (bool)m_slot; }
  bool                is_queued() const             { return m_time != timer(); }

  slot_void&          slot()                        { return m_slot; }

  const timer&        time() const                  { return m_time; }
  void                clear_time()                  { m_time = timer(); }
  void                set_time(const timer& t)      { m_time = t; }

  bool                compare(const timer& t) const { return m_time > t; }

private:
  priority_item(const priority_item&);
  void operator = (const priority_item&);

  timer               m_time;
  slot_void           m_slot;
};

struct priority_compare {
  bool operator () (const priority_item* const p1, const priority_item* const p2) const {
    return p1->time() > p2->time();
  }
};

typedef std::equal_to<priority_item*> priority_equal;
typedef priority_queue<priority_item*, priority_compare, priority_equal> priority_queue_default;

inline void
priority_queue_perform(priority_queue_default* queue, timer t) {
  while (!queue->empty() && queue->top()->time() <= t) {
    priority_item* v = queue->top();
    queue->pop();

    v->clear_time();
    v->slot()();
  }
}

inline void
priority_queue_insert(priority_queue_default* queue, priority_item* item, timer t) {
  if (t == timer())
    throw torrent::internal_error("priority_queue_insert(...) received a bad timer.");

  if (!item->is_valid())
    throw torrent::internal_error("priority_queue_insert(...) called on an invalid item.");

  if (item->is_queued())
    throw torrent::internal_error("priority_queue_insert(...) called on an already queued item.");

  if (queue->find(item) != queue->end())
    throw torrent::internal_error("priority_queue_insert(...) item found in queue.");

  item->set_time(t);
  queue->push(item);
}

inline void
priority_queue_erase(priority_queue_default* queue, priority_item* item) {
  if (!item->is_queued())
    return;

  // Check is_valid() after is_queued() so that it is safe to call
  // erase on untouched instances.
  if (!item->is_valid())
    throw torrent::internal_error("priority_queue_erase(...) called on an invalid item.");

  // Unqueue it before erasing.
  item->clear_time();

  if (!queue->erase(item))
    throw torrent::internal_error("priority_queue_erase(...) could not find item in queue.");
}

inline void
priority_queue_update(priority_queue_default* queue, priority_item* item, timer t) {
  if (t == timer())
    throw torrent::internal_error("priority_queue_insert(...) received a bad timer.");

  if (!item->is_valid())
    throw torrent::internal_error("priority_queue_insert(...) called on an invalid item.");

  if (queue->find(item) == queue->end()) {
    if (item->is_queued())
      throw torrent::internal_error("priority_queue_update(...) cannot insert an already queued item.");

    item->set_time(t);
    queue->push(item);

  } else {
    item->set_time(t);
    queue->update();
  }
}

}

#endif
