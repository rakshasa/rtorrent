#ifndef RTORRENT_DISPLAY_LAYERS_H
#define RTORRENT_DISPLAY_LAYERS_H

#include <list>

namespace display {

class Canvas;

class Layers : private std::list<Canvas*> {
public:
  typedef std::list<Canvas*> Base;

  using Base::iterator;
  using Base::const_iterator;
  using Base::reverse_iterator;
  using Base::const_reverse_iterator;

  using Base::begin;
  using Base::end;
  using Base::rbegin;
  using Base::rend;
  using Base::push_front;
  using Base::push_back;
  using Base::pop_front;
  using Base::pop_back;

  void do_update();
};

}

#endif
