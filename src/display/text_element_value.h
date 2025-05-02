#ifndef RTORRENT_DISPLAY_TEXT_ELEMENT_VALUE_H
#define RTORRENT_DISPLAY_TEXT_ELEMENT_VALUE_H

#include <cstring>

#include "text_element.h"

namespace display {

class TextElementValueBase : public TextElement {
public:
  static const int flag_normal    = 0;
  static const int flag_timer     = (1 << 0);
  static const int flag_date      = (1 << 1);
  static const int flag_time      = (1 << 2);

  static const int flag_kb        = (1 << 3);
  static const int flag_mb        = (1 << 4);
  static const int flag_xb        = (1 << 5);

  static const int flag_elapsed   = (1 << 8);
  static const int flag_remaining = (1 << 9);
  static const int flag_usec      = (1 << 10);

  int                 flags() const                 { return m_flags; }
  void                set_flags(int flags)          { m_flags = flags; }

  int                 attributes() const            { return m_attributes; }
  void                set_attributes(int a)         { m_attributes = a; }

  virtual char*       print(char* first, char* last, Canvas::attributes_list* attributes, rpc::target_type target);

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

  virtual extent_type max_length()                  { return 12; }

private:
  virtual int64_t     value([[maybe_unused]] void* object) { return m_value; }

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

  virtual extent_type max_length()        { return 12; }

private:
  virtual int64_t     value([[maybe_unused]] void* object) { return m_slot(); }

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

  virtual extent_type max_length()                  { return 12; }

private:
  virtual int64_t value(void* object) {
    if (object == NULL)
      return 0;

    return m_slot(reinterpret_cast<arg1_type>(object));
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
