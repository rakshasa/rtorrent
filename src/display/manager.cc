#include "config.h"

#include <stdexcept>
#include <algorithm>
#include <functional>

#include "canvas.h"
#include "manager.h"
#include "window.h"

namespace display {

template <typename Type, typename Ftor>
struct _accumulate {
  _accumulate(Type& t, Ftor f) : m_t(t), m_f(f) {}

  template <typename Arg>
  void operator () (Arg& a) { m_t += m_f(a); }

  Type& m_t;
  Ftor m_f;
};

template <typename Type, typename Ftor>
_accumulate<Type, Ftor> accumulate(Type& t, Ftor f) {
  return _accumulate<Type, Ftor>(t, f);
}

Manager::iterator
Manager::erase(Window* w) {
  iterator itr = std::find(begin(), end(), w);

  if (itr == end())
    throw std::logic_error("Manager::erase(...) did not find the window");

  return Base::erase(itr);
}

void
Manager::adjust_layout() {
  int countDynamic = 0;
  int staticHeight = 0;

  std::for_each(begin(), end(), accumulate(staticHeight, std::mem_fun(&Window::get_min_height)));
  std::for_each(begin(), end(), accumulate(countDynamic, std::mem_fun(&Window::is_dynamic)));

  int dynamic = std::max(0, Canvas::get_screen_height() - staticHeight);
  int height = 0, h;

  for (iterator itr = begin(); itr != end(); ++itr, height += h) {

    if ((*itr)->is_dynamic()) {
      dynamic -= h = (dynamic + countDynamic - 1) / countDynamic;
      countDynamic--;
    } else {
      h = 0;
    }

    h += (*itr)->get_min_height();

    (*itr)->resize(0, height, Canvas::get_screen_width(), h);
  }
}

void
Manager::do_update() {
  std::for_each(begin(), end(), std::mem_fun(&Window::redraw));
  std::for_each(begin(), end(), std::mem_fun(&Window::refresh));

  Canvas::do_update();
}

}
