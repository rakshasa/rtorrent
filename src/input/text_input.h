#ifndef RTORRENT_INPUT_TEXT_INPUT_H
#define RTORRENT_INPUT_TEXT_INPUT_H

#include <string>
#include <sigc++/slot.h>

namespace input {

class TextInput : private std::string {
public:
  typedef std::string       Base;
  typedef sigc::slot0<void> SlotDirty;

  using Base::c_str;
  using Base::empty;
  using Base::size;

  TextInput() : m_pos(0), m_alt(false) {}

  size_type get_pos()                  { return m_pos; }

  bool      pressed(int key);

  void      clear()                    { m_pos = 0; m_alt = false; Base::clear(); }

  void      slot_dirty(SlotDirty s)    { m_slotDirty = s; }

  const std::string& str()             { return *this; }

private:
  size_type m_pos;

  bool      m_alt;
  SlotDirty m_slotDirty;
};

}

#endif
