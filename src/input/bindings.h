#ifndef RTORRENT_INPUT_BINDINGS_H
#define RTORRENT_INPUT_BINDINGS_H

#include <map>
#include <ncurses.h>
#include <sigc++/slot.h>

namespace input {

class Bindings : private std::map<int, sigc::slot0<void> > {
public:
  typedef std::map<int, sigc::slot0<void> > Base;

  using Base::iterator;
  using Base::const_iterator;
  using Base::reverse_iterator;
  using Base::const_reverse_iterator;

  using Base::begin;
  using Base::end;
  using Base::rbegin;
  using Base::rend;
  using Base::find;

  using Base::operator[];

  Bindings() : m_active(true) {}

  void      activate() { m_active = true; }
  void      disable()  { m_active = false; }

  bool      pressed(int key);

private:
  bool      m_active;
};

}

#endif
