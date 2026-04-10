#include "config.h"

#include "display/text_element_string.h"

#include <torrent/utils/string_manip.h>

#include "rpc/parse_commands.h"

namespace display {

char*
TextElementStringBase::print(char* first, char* last, Canvas::attributes_list* attributes, rpc::target_type target) {
  Attributes baseAttribute = attributes->back();
  push_attribute(attributes, Attributes(first, m_attributes, Attributes::color_invalid));

  if (first == last)
    return first;

  if (m_flags & flag_escape_hex) {
    auto buffer = std::make_unique<char[]>(last - first);
    char* bufferLast = copy_string(buffer.get(), buffer.get() + (last - first), target);

    first = torrent::utils::transform_to_hex(buffer.get(), bufferLast, first, last);

  } else if (m_flags & flag_escape_html) {
    auto buffer = std::make_unique<char[]>(last - first);
    char* bufferLast = copy_string(buffer.get(), buffer.get() + (last - first), target);

    first = torrent::utils::copy_escape_html(buffer.get(), bufferLast, first, last);

  } else {
    first = copy_string(first, last, target);
  }

  push_attribute(attributes, Attributes(first, baseAttribute));

  return first;
}

char*
TextElementString::copy_string(char* first, char* last, [[maybe_unused]] rpc::target_type target) {
  extent_type length = std::min<extent_type>(last - first, m_string.size());

  std::memcpy(first, m_string.c_str(), length);

  return first + length;
}

char*
TextElementCString::copy_string(char* first, char* last, [[maybe_unused]] rpc::target_type target) {
  extent_type length = std::min<extent_type>(last - first, m_length);

  std::memcpy(first, m_string, length);

  return first + length;
}

char*
TextElementCommand::print(char* first, char* last, Canvas::attributes_list* attributes, rpc::target_type target) {
  Attributes baseAttribute = attributes->back();
  push_attribute(attributes, Attributes(first, m_attributes, Attributes::color_invalid));

  torrent::Object result = rpc::parse_command(target, m_command, m_commandEnd).first;

  if (first == last)
    return first;

  switch (result.type()) {
  case torrent::Object::TYPE_STRING:
  {
    const std::string& str = result.as_string();

    if (m_flags & flag_escape_hex) {
      first = torrent::utils::transform_to_hex(str.c_str(), str.c_str() + str.size(), first, last);

    } else if (m_flags & flag_escape_html) {
      first = torrent::utils::copy_escape_html(str.c_str(), str.c_str() + str.size(), first, last);

    } else {
      size_t length = std::min<size_t>(str.size(), std::distance(first, last));

      std::memcpy(first, str.c_str(), length);
      first += std::min<size_t>(str.size(), length);
    }

    break;
  }
  case torrent::Object::TYPE_VALUE:
  {
    first += std::min<ptrdiff_t>(std::max(snprintf(first, last - first + 1, "%lld", (long long int)result.as_value()), 0), last - first + 1);
    break;
  }
  default:
    return first;
  }

  push_attribute(attributes, Attributes(first, baseAttribute));
  return first;
}

}
