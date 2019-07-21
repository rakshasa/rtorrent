// rTorrent - BitTorrent client
// Copyright (C) 2005-2011, Jari Sundell
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// In addition, as a special exception, the copyright holders give
// permission to link the code of portions of this program with the
// OpenSSL library under certain conditions as described in each
// individual source file, and distribute linked combinations
// including the two.
//
// You must obey the GNU General Public License in all respects for
// all of the code used other than OpenSSL.  If you modify file(s)
// with this exception, you may extend this exception to your version
// of the file(s), but you are not obligated to do so.  If you do not
// wish to do so, delete this exception statement from your version.
// If you delete this exception statement from all source files in the
// program, then also delete it here.
//
// Contact:  Jari Sundell <jaris@ifi.uio.no>
//
//           Skomakerveien 33
//           3185 Skoppum, NORWAY

#ifndef RTORRENT_DISPLAY_TEXT_ELEMENT_STRING_H
#define RTORRENT_DISPLAY_TEXT_ELEMENT_STRING_H

#include <iterator>
#include <string>
#include <cstring>

#include "text_element.h"

namespace display {

class TextElementStringBase : public TextElement {
public:
  static const int flag_normal      = 0;
  static const int flag_escape_hex  = (1 << 0);
  static const int flag_escape_html = (1 << 1);

  static const int flag_fixed_width = (1 << 8);

  int                 flags() const                 { return m_flags; }
  void                set_flags(int flags)          { m_flags = flags; }

  int                 attributes() const            { return m_attributes; }
  void                set_attributes(int a)         { m_attributes = a; }

  virtual char*       print(char* first, char* last, Canvas::attributes_list* attributes, rpc::target_type target);

protected:
  virtual char*       copy_string(char* first, char* last, rpc::target_type target) = 0;

  int                 m_flags;
  int                 m_attributes;
};

class TextElementString : public TextElementStringBase {
public:
  TextElementString(const std::string& s, int flags = flag_normal, int attributes = Attributes::a_invalid) : m_string(s) {
    m_flags = flags;
    m_attributes = attributes;
  }

  const std::string&  str() const                   { return m_string; }
  void                set_str(const std::string& s) { m_string = s; }

  virtual extent_type max_length()                  { return m_string.size(); }

private:
  virtual char*       copy_string(char* first, char* last, rpc::target_type target);

  std::string         m_string;
};

class TextElementCString : public TextElementStringBase {
public:
  TextElementCString(const char* s, int flags = flag_normal, int attributes = Attributes::a_invalid) : m_length(std::strlen(s)), m_string(s) {
    m_flags = flags;
    m_attributes = attributes;
  }

  virtual extent_type max_length()                  { return m_length; }

private:
  virtual char*       copy_string(char* first, char* last, rpc::target_type target);

  extent_type         m_length;
  const char*         m_string;
};

template <typename slot_type>
class TextElementStringSlot : public TextElementStringBase {
public:
  typedef typename slot_type::argument_type arg1_type;
  typedef typename slot_type::result_type   result_type;

  TextElementStringSlot(const slot_type& slot, int flags, int attributes, extent_type length) : m_length(length), m_slot(slot) {
    m_flags = flags;
    m_attributes = attributes;
  }

  virtual extent_type max_length()                  { return m_length; }

private:
  virtual char* copy_string(char* first, char* last, rpc::target_type target) {
    if (target.second == nullptr)
      return first;

    result_type result = m_slot(reinterpret_cast<arg1_type>(target.second));
    extent_type length = std::min<extent_type>(result_length(&result), last - first);

    std::memcpy(first, result_buffer(&result), length);

    return first + length;
  }

  template <typename Result>
  extent_type result_length(Result* result)      { return std::min<extent_type>(result->size(), m_length); }

  extent_type result_length(const char** result) {
    if (m_flags & flag_fixed_width)
      return m_length;
    else
      return std::min<extent_type>(std::strlen(*result), m_length);
  }

  template <typename Result>
  const char* result_buffer(Result* result)      { return result->c_str(); }
  const char* result_buffer(const char** result) { return *result; }

  extent_type         m_length;
  slot_type           m_slot;
};

template <typename slot_type>
inline TextElementStringSlot<slot_type>*
text_element_string_slot(const slot_type& slot,
                         int flags = TextElementStringBase::flag_normal,
                         int attributes = Attributes::a_invalid,
                         TextElement::extent_type length = TextElement::extent_full) {
  return new TextElementStringSlot<slot_type>(slot, flags, attributes, length);
}

//
// New TE's for calling commands directly. Move to a better place.
//

class TextElementCommand : public TextElement {
public:
  static const int flag_normal      = 0;
  static const int flag_escape_hex  = (1 << 0);
  static const int flag_escape_html = (1 << 1);

  static const int flag_fixed_width = (1 << 8);

  TextElementCommand(const char* command, int flags, int attributes, extent_type length) :
    m_flags(flags),
    m_attributes(attributes),
    m_length(length),
    m_command(command),
    m_commandEnd(command + std::strlen(command)) {}

  int                 flags() const                 { return m_flags; }
  void                set_flags(int flags)          { m_flags = flags; }

  int                 attributes() const            { return m_attributes; }
  void                set_attributes(int a)         { m_attributes = a; }

  virtual char*       print(char* first, char* last, Canvas::attributes_list* attributes, rpc::target_type target);

  virtual extent_type max_length()                  { return m_length; }

protected:
  int                 m_flags;
  int                 m_attributes;
  extent_type         m_length;

  const char*         m_command;
  const char*         m_commandEnd;
};

namespace helpers {

inline TextElementCommand*
te_command(const char* command, int flags = TextElementCommand::flag_normal, int attributes = Attributes::a_invalid) {
  return new TextElementCommand(command, flags, attributes, TextElementCommand::extent_full);
}

}

}

#endif
