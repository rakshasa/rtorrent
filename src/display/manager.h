#ifndef RTORRENT_DISPLAY_MANAGER_H
#define RTORRENT_DISPLAY_MANAGER_H

#include <list>
#include "manager_element.h"

namespace display {

class Manager : private std::list<ManagerElement> {
public:
  typedef std::list<ManagerElement> Base;

  using Base::iterator;
  using Base::const_iterator;
  using Base::reverse_iterator;
  using Base::const_reverse_iterator;

  using Base::begin;
  using Base::end;
  using Base::rbegin;
  using Base::rend;

  enum {
    NEW_ROW = 0x1
  };

  Base::iterator add(Window* w, int flags = 0)          { return Base::insert(end(), ManagerElement(w, flags)); }

  void           adjust_layout();
  void           do_update();

private:
  // Functions for adjusting ranges in different directions.
  void           adjust_row(iterator bItr, iterator eItr, int x, int y, int w, int h);
};

}

#endif
