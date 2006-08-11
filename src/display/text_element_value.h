// rTorrent - BitTorrent client
// Copyright (C) 2005-2006, Jari Sundell
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

#ifndef RTORRENT_DISPLAY_TEXT_ELEMENT_VALUE_H
#define RTORRENT_DISPLAY_TEXT_ELEMENT_VALUE_H

#include <cstring>
#include <inttypes.h>

#include "text_element.h"

namespace display {

class TextElementValueBase : public TextElement {
public:
  static const int flag_normal    = (1 << 0);
  static const int flag_timer     = (1 << 1);
  static const int flag_date      = (1 << 2);
  static const int flag_time      = (1 << 3);

  static const int flag_kb        = (1 << 4);
  static const int flag_mb        = (1 << 5);

  static const int flag_elapsed   = (1 << 8);
  static const int flag_remaining = (1 << 9);
  static const int flag_usec      = (1 << 10);

  int                 flags() const                 { return m_flags; }
  void                set_flags(int flags)          { m_flags = flags; }

  int                 attributes() const            { return m_attributes; }
  void                set_attributes(int a)         { m_attributes = a; }

  virtual char*       print(char* first, const char* last, Canvas::attributes_list* attributes, void* object);

protected:
  virtual int64_t     value(void* object) = 0;

  int                 m_flags;
  int                 m_attributes;
};

class TextElementValue : public TextElementValueBase {
public:
  TextElementValue(int64_t value, int flags = flag_normal, int attributes = Attributes::a_invalid) : m_value(value) {
    m_flags = flags;
    m_attributes = attributes;
  }

  int64_t             value() const                 { return m_value; }
  void                set_value(int64_t v)          { m_value = v; }

private:
  virtual extent_type max_length()                  { return 12; }

  virtual int64_t     value(void* object)           { return m_value; }

  int64_t             m_value;
};

template <typename slot_type>
class TextElementValueSlot0 : public TextElementValueBase {
public:
  typedef typename slot_type::result_type   result_type;

  TextElementValueSlot0(const slot_type& slot, int flags = flag_normal, int attributes = Attributes::a_invalid) : m_slot(slot) {
    m_flags = flags;
    m_attributes = attributes;
  }

private:
  virtual extent_type max_length()        { return 12; }

  virtual int64_t     value(void* object) { return m_slot(); }

  slot_type           m_slot;
};

template <typename slot_type>
class TextElementValueSlot : public TextElementValueBase {
public:
  typedef typename slot_type::argument_type arg1_type;
  typedef typename slot_type::result_type   result_type;

  TextElementValueSlot(const slot_type& slot, int flags = flag_normal, int attributes = Attributes::a_invalid) : m_slot(slot) {
    m_flags = flags;
    m_attributes = attributes;
  }

private:
  virtual extent_type max_length()                  { return 12; }

  virtual int64_t value(void* object) {
    arg1_type arg1 = reinterpret_cast<arg1_type>(object);

    if (arg1 == NULL)
      return 0;

    return m_slot(arg1);
  }

  slot_type           m_slot;
};

template <typename slot_type>
inline TextElementValueSlot0<slot_type>*
text_element_value_void(const slot_type& slot, int flags = TextElementValueBase::flag_normal, int attributes = Attributes::a_invalid) {
  return new TextElementValueSlot0<slot_type>(slot, flags, attributes);
}

template <typename slot_type>
inline TextElementValueSlot<slot_type>*
text_element_value_slot(const slot_type& slot, int flags = TextElementValueBase::flag_normal, int attributes = Attributes::a_invalid) {
  return new TextElementValueSlot<slot_type>(slot, flags, attributes);
}

}

#endif
