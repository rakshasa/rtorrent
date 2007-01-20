// rTorrent - BitTorrent client
// Copyright (C) 2006, Jari Sundell
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

#include "config.h"

#include <stdlib.h>

#include "variable_generic.h"

namespace utils {

const torrent::Object& VariableAny::get() { return m_variable; }
void VariableAny::set(const torrent::Object& arg) { m_variable = arg; }

void
VariableValue::set(const torrent::Object& arg) {
  int64_t value;

  switch (arg.type()) {
  case torrent::Object::TYPE_NONE:
    m_variable = (int64_t)0;
    break;

  case torrent::Object::TYPE_VALUE:
    m_variable = arg;
    break;

  case torrent::Object::TYPE_STRING:
    string_to_value_unit(arg.as_string().c_str(), &value, 0, 1);

    m_variable = value;
    break;

  default:
    throw torrent::input_error("VariableValue unsupported type restriction.");
  }
}

void
VariableBool::set(const torrent::Object& arg) {
  switch (arg.type()) {
  case torrent::Object::TYPE_VALUE:
    m_variable = arg.as_value() ? (int64_t)1 : (int64_t)0;
    break;

  case torrent::Object::TYPE_STRING:
    // Move the checks into some is_true, is_false think in Variable.
    if (arg.as_string() == "yes" || arg.as_string() == "true")
      m_variable = (int64_t)1;

    else if (arg.as_string() == "no" || arg.as_string() == "false")
      m_variable = (int64_t)0;

    else
      throw torrent::input_error("String does not parse as a boolean.");

    break;

  default:
    throw torrent::input_error("Input is not a boolean.");
  }
}

const torrent::Object&
VariableObject::get_d(core::Download* download) {
  if (m_root.empty())
    return download->bencode()->get_key(m_key);
  else
    return download->bencode()->get_key(m_root).get_key(m_key);
}

void
VariableObject::set_d(core::Download* download, const torrent::Object& arg) {
  // Consider removing if TYPE_NONE.
  torrent::Object* root;

  if (m_root.empty())
    root = download->bencode();
  else
    root = &download->bencode()->get_key(m_root);

  switch (m_type) {
  case torrent::Object::TYPE_NONE:
    root->insert_key(m_key, arg);
    break;

  case torrent::Object::TYPE_STRING:
    if (arg.is_string())
      root->insert_key(m_key, arg);
    else
      throw torrent::input_error("VariableObject could not convert to string.");
      
    break;

  case torrent::Object::TYPE_VALUE:
    if (arg.is_value())
      root->insert_key(m_key, arg);
    else
      throw torrent::input_error("VariableObject could not convert to value.");

    break;

  default:
    throw torrent::input_error("VariableObject unsupported type restriction.");
  }
}

const torrent::Object& VariableDownload::get() { return m_global->get(); }
const torrent::Object& VariableDownload::get_d(core::Download* download) { return m_download->get_d(download); }
void VariableDownload::set(const torrent::Object& arg) { m_global->set(arg); }
void VariableDownload::set_d(core::Download* download, const torrent::Object& arg) { m_download->set_d(download, arg); }

// 
// New and prettified.
//

const torrent::Object&
VariableVoidSlot::get() {
  return m_cache;
}

void
VariableVoidSlot::set(const torrent::Object& arg) {
  if (!m_slotSet.is_valid())
    return;

  m_slotSet();
}

const torrent::Object&
VariableValueSlot::get() {
  if (!m_slotGet.is_valid())
    return m_cache = torrent::Object();

  m_cache = m_slotGet() / m_unit;

  return m_cache;
}

void
VariableValueSlot::set(const torrent::Object& arg) {
  if (!m_slotSet.is_valid())
    return;

  value_type value;

  switch (arg.type()) {
  case torrent::Object::TYPE_STRING:
    string_to_value_unit(arg.as_string().c_str(), &value, m_base, m_unit);

    // Check if we hit the end of the input.

    m_slotSet(value);
    break;

  case torrent::Object::TYPE_VALUE:
    m_slotSet(arg.as_value());
    break;

  default:
    throw torrent::input_error("Not a value");
  }
}

const torrent::Object&
VariableDownloadValueSlot::get_d(core::Download* download) {
  // Should clear the cache.
  if (!m_slotGetDownload.is_valid())
    return m_cache = torrent::Object();

  return m_cache = m_slotGetDownload(download) / m_unit;
}

void
VariableDownloadValueSlot::set_d(core::Download* download, const torrent::Object& arg) {
  if (!m_slotSetDownload.is_valid())
    return;

  value_type value;

  switch (arg.type()) {
  case torrent::Object::TYPE_STRING:
    string_to_value_unit(arg.as_string().c_str(), &value, m_base, m_unit);

    // Check if we hit the end of the input.

    m_slotSetDownload(download, value);
    break;

  case torrent::Object::TYPE_VALUE:
    m_slotSetDownload(download, arg.as_value());
    break;

  default:
    throw torrent::input_error("Not a value");
  }
}

const torrent::Object&
VariableStringSlot::get() {
  if (!m_slotGet.is_valid())
    return m_cache = torrent::Object();

  return m_cache = m_slotGet();
}

void
VariableStringSlot::set(const torrent::Object& arg) {
  if (!m_slotSet.is_valid())
    return;

  switch (arg.type()) {
  case torrent::Object::TYPE_STRING:
    m_slotSet(arg.as_string());
    break;
  case torrent::Object::TYPE_NONE:
    m_slotSet(std::string());
    break;
  default:
    throw torrent::input_error("Not a string.");
  }
}

const torrent::Object&
VariableDownloadStringSlot::get_d(core::Download* download) {
  // Should clear the cache.
  if (!m_slotGetDownload.is_valid())
    return m_cache = torrent::Object();

  return m_cache = m_slotGetDownload(download);
}

void
VariableDownloadStringSlot::set_d(core::Download* download, const torrent::Object& arg) {
  if (!m_slotSetDownload.is_valid())
    return;

  switch (arg.type()) {
  case torrent::Object::TYPE_STRING:
    m_slotSetDownload(download, arg.as_string());
    break;
  case torrent::Object::TYPE_NONE:
    m_slotSetDownload(download, std::string());
    break;
  default:
    throw torrent::input_error("Not a string.");
  }
}

}
