#ifndef RTORRENT_DISPLAY_MANAGER_H
#define RTORRENT_DISPLAY_MANAGER_H

#include <list>

namespace display {

class Window;

class Manager : private std::list<Window*> {
public:
  typedef std::list<Window*> Base;

  using Base::iterator;
  using Base::const_iterator;
  using Base::reverse_iterator;
  using Base::const_reverse_iterator;

  using Base::begin;
  using Base::end;
  using Base::rbegin;
  using Base::rend;

  using Base::insert;
  using Base::erase;
  using Base::push_front;
  using Base::push_back;

  iterator       erase(Window* w);

  iterator       find(Window* w);

  void           adjust_layout();
  void           do_update();

private:
};

}

#endif
