#ifndef RTORRENT_DISPLAY_MANAGER_H
#define RTORRENT_DISPLAY_MANAGER_H

#include <vector>

namespace display {

class Window;

class Manager : private std::vector<Window*> {
public:
  typedef std::vector<Window*> Base;

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
  using Base::push_back;

  void           adjust_layout();
  void           do_update();

private:
};

}

#endif
