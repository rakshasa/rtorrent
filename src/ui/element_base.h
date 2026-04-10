#ifndef RTORRENT_UI_ELEMENT_BASE_H
#define RTORRENT_UI_ELEMENT_BASE_H

#include "input/bindings.h"

namespace display {
  class Frame;
  class Window;
}

namespace ui {

class ElementBase {
public:
  typedef std::function<void ()> slot_type;

  virtual ~ElementBase() = default;

  bool                is_active() const { return m_frame != NULL; }

  input::Bindings&    bindings()        { return m_bindings; }

  virtual void        activate(display::Frame* frame, bool focus = true) = 0;
  virtual void        disable() = 0;

  void                slot_exit(const slot_type& s) { m_slot_exit = s; }

  void                mark_dirty();

protected:
  display::Frame*     m_frame{};
  bool                m_focus{};

  input::Bindings     m_bindings;
  slot_type           m_slot_exit;
};

// Return a range with a distance of no more than __distance and
// between __first and __last, centered on __middle1.
template <typename _InputIter, typename _Distance>
std::pair<_InputIter, _InputIter>
advance_bidirectional(_InputIter __first, _InputIter __middle1, _InputIter __last, _Distance __distance) {
  _InputIter __middle2 = __middle1;

  do {
    if (!__distance)
      break;

    if (__middle2 != __last) {
      ++__middle2;
      --__distance;

    } else if (__middle1 == __first) {
      break;
    }

    if (!__distance)
      break;

    if (__middle1 != __first) {
      --__middle1;
      --__distance;

    } else if (__middle2 == __last) {
      break;
    }

  } while (true);

  return std::make_pair(__middle1, __middle2);
}

template <typename _InputIter, typename _Distance>
_InputIter
advance_forward(_InputIter __first, _InputIter __last, _Distance __distance) {
  while (__first != __last && __distance != 0) {
    __first++;
    __distance--;
  }

  return __first;
}

template <typename _InputIter, typename _Distance>
_InputIter
advance_backward(_InputIter __first, _InputIter __last, _Distance __distance) {
  while (__first != __last && __distance != 0) {
    __first--;
    __distance--;
  }

  return __first;
}


}

#endif
