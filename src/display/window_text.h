#ifndef RTORRENT_DISPLAY_WINDOW_TEXT_H
#define RTORRENT_DISPLAY_WINDOW_TEXT_H

#include <vector>

#include "text_element.h"
#include "window.h"

namespace display {

class WindowText : public Window, public std::vector<TextElement*> {
public:
  typedef std::vector<TextElement*>   base_type;

  typedef base_type::value_type       value_type;
  typedef base_type::reference        reference;
  typedef base_type::iterator         iterator;
  typedef base_type::const_iterator   const_iterator;
  typedef base_type::reverse_iterator reverse_iterator;

  using base_type::empty;
  using base_type::size;

  using base_type::begin;
  using base_type::end;
  using base_type::rbegin;
  using base_type::rend;

  WindowText(rpc::target_type target = rpc::make_target(), extent_type margin = 0);
  ~WindowText() { clear(); }

  void                clear();

  rpc::target_type    target() const                      { return m_target; }
  void                set_target(rpc::target_type target) { m_target = target; }

  uint32_t            interval() const         { return m_interval; }
  void                set_interval(uint32_t i) { m_interval = i; }

  void                push_back(TextElement* element);

  // Set an error handler if targets pointing to NULL elements should
  // be handled separately to avoid throwing errors.
  void                set_error_handler(TextElement* element) { m_errorHandler = element; }

  virtual void        redraw();

private:
  rpc::target_type    m_target;
  TextElement*        m_errorHandler{};

  extent_type         m_margin;
  uint32_t            m_interval{};
};

}

#endif
