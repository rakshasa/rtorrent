#ifndef RTORRENT_WINDOW_BASE_H
#define RTORRENT_WINDOW_BASE_H

#include <functional>
#include <torrent/utils/scheduler.h>
#include <torrent/system/thread.h>

#include "canvas.h"
#include "globals.h"

namespace display {

class Canvas;

class Window {
public:
  typedef uint32_t extent_type;

  typedef std::function<void()>                                   Slot;
  typedef std::function<void(Window*)>                            SlotWindow;
  typedef std::function<void(Window*, std::chrono::microseconds)> SlotTimer;

  static const int flag_active    = (1 << 0);
  static const int flag_offscreen = (1 << 1);
  static const int flag_focused   = (1 << 2);
  static const int flag_left      = (1 << 3);
  static const int flag_bottom    = (1 << 4);

  static const extent_type extent_static = extent_type();
  static const extent_type extent_full   = ~extent_type();

  Window(Canvas* canvas, int flags,
         extent_type minWidth, extent_type minHeight,
         extent_type maxWidth, extent_type maxHeight);

  virtual ~Window();

  bool                is_active() const                    { return m_flags & flag_active; }
  void                set_active(bool state);

  bool                is_offscreen() const                 { return m_flags & flag_offscreen; }
  void                set_offscreen(bool state)            { if (state) m_flags |= flag_offscreen; else m_flags &= ~flag_offscreen; }

  bool                is_focused() const                   { return m_flags & flag_focused; }
  void                set_focused(bool state)              { if (state) m_flags |= flag_focused; else m_flags &= ~flag_focused; }

  bool                is_left() const                      { return m_flags & flag_left; }
  void                set_left(bool state)                 { if (state) m_flags |= flag_left; else m_flags &= ~flag_left; }

  bool                is_bottom() const                    { return m_flags & flag_bottom; }
  void                set_bottom(bool state)               { if (state) m_flags |= flag_bottom; else m_flags &= ~flag_bottom; }

  bool                is_width_dynamic() const             { return m_max_width > m_min_width; }
  bool                is_height_dynamic() const            { return m_max_height > m_min_height; }

  // Do not call mark_dirty() from withing redraw() as it may cause
  // infinite looping in the display scheduler.
  bool                is_dirty()                           { return m_task_update.is_scheduled(); }
  void                mark_dirty()                         { if (!is_active()) return; m_slot_schedule(this, torrent::this_thread::cached_time()); }

  extent_type         min_width() const                    { return m_min_width; }
  extent_type         min_height() const                   { return m_min_height; }

  extent_type         max_width() const                    { return std::max(m_max_width, m_min_width); }
  extent_type         max_height() const                   { return std::max(m_max_height, m_min_height); }

  extent_type         width() const                        { return m_canvas->width(); }
  extent_type         height() const                       { return m_canvas->height(); }

  void                refresh()                            { m_canvas->refresh(); }
  void                resize(int x, int y, int w, int h);

  virtual void        redraw() = 0;

  auto                task_update()                        { return &m_task_update; }

  // Slot for adjust and refresh.
  static void         slot_schedule(SlotTimer s)           { m_slot_schedule = s; }
  static void         slot_unschedule(SlotWindow s)        { m_slot_unschedule = s; }
  static void         slot_adjust(Slot s)                  { m_slot_adjust = s; }

protected:
  Window(const Window&);
  void operator = (const Window&);

  void                schedule_update(unsigned int wait_seconds = 0);

  static SlotTimer    m_slot_schedule;
  static SlotWindow   m_slot_unschedule;
  static Slot         m_slot_adjust;

  std::unique_ptr<Canvas> m_canvas;

  int                 m_flags;

  extent_type         m_min_width;
  extent_type         m_min_height;

  extent_type         m_max_width;
  extent_type         m_max_height;

  torrent::utils::SchedulerEntry m_task_update;
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

