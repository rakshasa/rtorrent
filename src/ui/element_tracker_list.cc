#include "config.h"

#include <torrent/exceptions.h>
#include <torrent/tracker_list.h>
#include <torrent/tracker/tracker.h>

#include "display/frame.h"
#include "display/window_tracker_list.h"
#include "input/manager.h"

#include "control.h"
#include "element_tracker_list.h"

namespace ui {

ElementTrackerList::ElementTrackerList(core::Download* d) :
  m_download(d),
  m_window(NULL),
  m_focus(0) {

  m_bindings[KEY_LEFT] = m_bindings['B' - '@'] = std::bind(&slot_type::operator(), &m_slot_exit);

  m_bindings[' ']      = std::bind(&ElementTrackerList::receive_cycle_group, this);
  m_bindings['*']      = std::bind(&ElementTrackerList::receive_disable, this);

  m_bindings[KEY_DOWN] = m_bindings['N' - '@'] = std::bind(&ElementTrackerList::receive_next, this);
  m_bindings[KEY_UP]   = m_bindings['P' - '@'] = std::bind(&ElementTrackerList::receive_prev, this);
}

void
ElementTrackerList::activate(display::Frame* frame, bool focus) {
  if (m_window != NULL)
    throw torrent::internal_error("ui::ElementTrackerList::activate(...) is_active().");

  if (focus)
    control->input()->push_back(&m_bindings);

  m_window = new WTrackerList(m_download, &m_focus);
  m_window->set_active(true);
  m_window->set_focused(focus);

  m_frame = frame;
  m_frame->initialize_window(m_window);
}

void
ElementTrackerList::disable() {
  if (m_window == NULL)
    throw torrent::internal_error("ui::ElementTrackerList::disable(...) !is_active().");

  control->input()->erase(&m_bindings);

  m_frame->clear();
  m_frame = NULL;

  delete m_window;
  m_window = NULL;
}

display::Window*
ElementTrackerList::window() {
  return m_window;
}

void
ElementTrackerList::receive_disable() {
  if (m_window == NULL)
    throw torrent::internal_error("ui::ElementTrackerList::receive_disable(...) called on a disabled object");

  auto t = m_download->download()->tracker_list()->at(m_focus);

  if (t.is_enabled())
    t.disable();
  else
    t.enable();

  m_window->mark_dirty();
}

void
ElementTrackerList::receive_next() {
  if (m_window == NULL)
    throw torrent::internal_error("ui::ElementTrackerList::receive_next(...) called on a disabled object");

  if (++m_focus >= m_download->download()->tracker_list()->size())
    m_focus = 0;

  m_window->mark_dirty();
}

void
ElementTrackerList::receive_prev() {
  if (m_window == NULL)
    throw torrent::internal_error("ui::ElementTrackerList::receive_prev(...) called on a disabled object");

  if (m_download->download()->tracker_list()->size() == 0)
    return;

  if (m_focus != 0)
    --m_focus;
  else
    m_focus = m_download->download()->tracker_list()->size() - 1;

  m_window->mark_dirty();
}

void
ElementTrackerList::receive_cycle_group() {
  if (m_window == NULL)
    throw torrent::internal_error("ui::ElementTrackerList::receive_group_cycle(...) called on a disabled object");

  torrent::TrackerList* tl = m_download->tracker_list();

  if (m_focus >= tl->size())
    throw torrent::internal_error("ui::ElementTrackerList::receive_group_cycle(...) called with an invalid focus");

  tl->cycle_group(tl->at(m_focus).group());

  m_window->mark_dirty();
}

}
