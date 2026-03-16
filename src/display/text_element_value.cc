#include "config.h"

#include "display/text_element_value.h"

#include <ctime>

#include <torrent/utils/chrono.h>

#include "globals.h"

namespace display {

// Should be in text_element.cc.
void
TextElement::push_attribute(Canvas::attributes_list* attributes, Attributes value) {
  Attributes base = attributes->back();

  if (value.colors() == Attributes::color_invalid)
    value.set_colors(base.colors());

  if (value.attributes() == Attributes::a_invalid)
    value.set_attributes(base.attributes());

  if (base.position() == value.position())
    attributes->back() = value;
  else if (base.colors() != value.colors() || base.attributes() != value.attributes())
    attributes->push_back(value);
}

char*
TextElementValueBase::print(char* first, char* last, Canvas::attributes_list* attributes, rpc::target_type target) {
  Attributes baseAttribute = attributes->back();
  push_attribute(attributes, Attributes(first, m_attributes, Attributes::color_invalid));

  int64_t val = value(target.second);

  // Transform the value if needed.
  if (m_flags & flag_elapsed)
    val = torrent::this_thread::cached_seconds().count() - val;
  else if (m_flags & flag_remaining)
    val = val - torrent::this_thread::cached_seconds().count();

  if (m_flags & flag_usec)
    val = torrent::utils::cast_seconds(std::chrono::microseconds(val)).count();

  // Print the value.
  if (first == last) {
    // Do nothing, but ensure that the last attributes are set.

  } else if (m_flags & flag_kb) {
    // Just use a default width of 5 for now.
    first += std::min<ptrdiff_t>(std::max(snprintf(first, last - first + 1, "%5.1f", (double)val / (1 << 10)), 0), last - first + 1);

  } else if (m_flags & flag_mb) {
    // Just use a default width of 8 for now.
    first += std::min<ptrdiff_t>(std::max(snprintf(first, last - first + 1, "%8.1f", (double)val / (1 << 20)), 0), last - first + 1);

  } else if (m_flags & flag_xb) {

    if (val < (int64_t(1000) << 10))
      first += std::min<ptrdiff_t>(std::max(snprintf(first, last - first + 1, "%5.1f KB", (double)val / (int64_t(1) << 10)), 0), last - first + 1);
    else if (val < (int64_t(1000) << 20))
      first += std::min<ptrdiff_t>(std::max(snprintf(first, last - first + 1, "%5.1f MB", (double)val / (int64_t(1) << 20)), 0), last - first + 1);
    else if (val < (int64_t(1000) << 30))
      first += std::min<ptrdiff_t>(std::max(snprintf(first, last - first + 1, "%5.1f GB", (double)val / (int64_t(1) << 30)), 0), last - first + 1);
    else
      first += std::min<ptrdiff_t>(std::max(snprintf(first, last - first + 1, "%5.1f TB", (double)val / (int64_t(1) << 40)), 0), last - first + 1);

  } else if (m_flags & flag_timer) {
    if (val == 0)
      first += std::min<ptrdiff_t>(std::max(snprintf(first, last - first + 1, "--:--:--"), 0), last - first + 1);
    else
      first += std::min<ptrdiff_t>(std::max(snprintf(first, last - first + 1, "%2d:%02d:%02d", (int)(val / 3600), (int)((val / 60) % 60), (int)(val % 60)), 0), last - first + 1);

  } else if (m_flags & flag_date) {
    time_t t = val;
    std::tm *u = std::gmtime(&t);

    if (u == NULL)
      return first;

    first += std::min<ptrdiff_t>(std::max(snprintf(first, last - first + 1, "%02u/%02u/%04u", u->tm_mday, (u->tm_mon + 1), (1900 + u->tm_year)), 0), last - first + 1);

  } else if (m_flags & flag_time) {
    time_t t = val;
    std::tm *u = std::gmtime(&t);

    if (u == NULL)
      return first;

    first += std::min<ptrdiff_t>(std::max(snprintf(first, last - first + 1, "%2d:%02d:%02d", u->tm_hour, u->tm_min, u->tm_sec), 0), last - first + 1);

  } else {
    first += std::min<ptrdiff_t>(std::max(snprintf(first, last - first + 1, "%lld", (long long int)val), 0), last - first + 1);
  }

  push_attribute(attributes, Attributes(first, baseAttribute));

  return first;
}

}
