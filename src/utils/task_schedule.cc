#include "config.h"

#include <algorithm>
#include <functional>

#include "task.h"
#include "task_schedule.h"

namespace utils {

TaskSchedule::Container TaskSchedule::m_container;

// Remove this, replace with stuff.
struct task_comp {
  task_comp(Timer t) : m_time(t) {}

  bool operator () (Task* t) {
    return m_time <= t->get_time();
  }

  Timer m_time;
};

inline void
TaskSchedule::execute_task(Task* t) {
  t->clear_iterator();
  t->get_slot()();
}

void
TaskSchedule::perform(Timer t) {
  Container c;

  c.splice(c.begin(), c, m_container.begin(), std::find_if(m_container.begin(), m_container.end(), task_comp(t)));

  std::for_each(c.begin(), c.end(), std::ptr_fun(&TaskSchedule::execute_task));
}

Timer
TaskSchedule::get_timeout() {
  if (!m_container.empty())
    return std::max(m_container.front()->get_time() - Timer::current(), Timer());
  else
    return Timer((int64_t)(1 << 30) * 1000000);
}

TaskSchedule::iterator
TaskSchedule::insert(Task* t) {
  iterator itr = std::find_if(m_container.begin(), m_container.end(), task_comp(t->get_time()));

  return m_container.insert(itr, t);
}
			       
}
