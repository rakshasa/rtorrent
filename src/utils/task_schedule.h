#ifndef RTORRENT_UTILS_TASK_SCHEDULE_H
#define RTORRENT_UTILS_TASK_SCHEDULE_H

#include <list>

#include "timer.h"

namespace utils {

class Task;

class TaskSchedule {
public:
  friend class Task;

  typedef std::list<Task*>    Container;
  typedef Container::iterator iterator;

  static void         perform(Timer t);

  static Timer        get_timeout();

protected:
  static iterator     end()               { return m_container.end(); }

  static iterator     insert(Task* t);
  static void         erase(iterator itr) { m_container.erase(itr); }

private:
  static inline void  execute_task(Task* t);

  static Container    m_container;
};

}

#endif
