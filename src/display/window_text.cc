#include "config.h"

#include <algorithm>

#include "canvas.h"
#include "utils.h"
#include "window_text.h"

namespace display {

WindowText::WindowText(rpc::target_type target, extent_type margin) :
    Window(new Canvas, 0, 0, 0, extent_static, extent_static),
    m_target(target),
    m_margin(margin) {
}

void
WindowText::clear() {
  for (auto t : *this)
    delete t;
  base_type::clear();

  delete m_errorHandler;
  m_errorHandler = NULL;
}

void
WindowText::push_back(TextElement* element) {
  base_type::push_back(element);

//   m_min_height = size();
  m_max_height = size();

  if (element != NULL) {
    extent_type width = element->max_length();

    if (width == extent_full)
      m_max_width = extent_full;
    else
      m_max_width = std::max(m_max_width, element->max_length() + m_margin);
  }

  // Check if active, if so do the update thingie. Or be lazy?
}

void
WindowText::redraw() {
  if (m_interval != 0)
    schedule_update(m_interval);

  m_canvas->erase();

  unsigned int position = 0;

  if (m_canvas->height() == 0)
    return;

  if (m_errorHandler != NULL && m_target.second == NULL) {
    std::string buffer(m_canvas->width() + 1, ' ');

    Canvas::attributes_list attributes;
    attributes.push_back(Attributes(buffer.data(), Attributes::a_normal, Attributes::color_default));

    char* last = m_errorHandler->print(buffer.data(), buffer.data() + m_canvas->width(), &attributes, m_target);

    m_canvas->print_attributes(0, position, buffer.data(), last, &attributes);
    return;
  }

  for (iterator itr = begin(); itr != end() && position < m_canvas->height(); ++itr, ++position) {
    if (*itr == NULL)
      continue;

    std::string buffer(m_canvas->width() + 1, ' ');

    Canvas::attributes_list attributes;
    attributes.push_back(Attributes(buffer.data(), Attributes::a_normal, Attributes::color_default));

    char* last = (*itr)->print(buffer.data(), buffer.data() + m_canvas->width(), &attributes, m_target);

    m_canvas->print_attributes(0, position, buffer.data(), last, &attributes);
  }
}

}
