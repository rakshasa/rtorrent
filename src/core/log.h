#ifndef RTORRENT_CORE_LOG_H
#define RTORRENT_CORE_LOG_H

#include <deque>
#include <string>
#include <sigc++/signal.h>

#include "utils/timer.h"

namespace core {

class Log : private std::deque<std::pair<utils::Timer, std::string> > {
public:
  typedef std::pair<utils::Timer, std::string> Type;
  typedef std::deque<Type>                     Base;
  typedef sigc::signal0<void>                  Signal;

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

  void      push_front(const std::string& msg);

  iterator  find_older(utils::Timer t);

  Signal&   signal_update() { return m_signalUpdate; }

private:
  Signal    m_signalUpdate;
};

}

#endif
