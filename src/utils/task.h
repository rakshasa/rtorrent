#ifndef RTORRENT_UTILS_TASK_H
#define RTORRENT_UTILS_TASK_H

#include <sigc++/slot.h>

#include "timer.h"
#include "task_schedule.h"

namespace utils {

class Task {
public:
  typedef sigc::slot<void> Slot;

  Task(Slot s = Slot()) : m_slot(s)       { clear_iterator(); }
  ~Task()                                 { remove(); }

  bool  is_scheduled()                    { return m_itr != TaskSchedule::end(); }

  void  set_slot(Slot s)                  { m_slot = s; }
  Slot& get_slot()                        { return m_slot; }

  Timer get_time()                        { return m_time; }

  void  insert(Timer t) {
    remove();

    m_time = t;
    m_itr = TaskSchedule::insert(this);
  }

  void  remove() {
    if (m_itr != TaskSchedule::end()) {
      TaskSchedule::erase(m_itr);
      clear_iterator();
    }
  }

protected:
  friend class TaskSchedule;

  TaskSchedule::iterator get_iterator()   { return m_itr; }
  void                   clear_iterator() { m_itr = TaskSchedule::end(); }

private:
  Task(const Task&);
  void operator () (const Task&);

  Timer                  m_time;
  TaskSchedule::iterator m_itr;
  Slot                   m_slot;
};

}

#endif
