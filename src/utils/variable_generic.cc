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

#include <cstdlib>

#include "variable_generic.h"

namespace utils {

VariableAny::~VariableAny() {
}

const torrent::Bencode&
VariableAny::get() {
  return m_variable;
}

void
VariableAny::set(const torrent::Bencode& arg) {
  m_variable = arg;
}

VariableValue::~VariableValue() {
}

const torrent::Bencode&
VariableValue::get() {
  return m_variable;
}

void
VariableValue::set(const torrent::Bencode& arg) {
  uint64_t value;
  const char* first;
  char* last;

  switch (arg.get_type()) {
  case torrent::Bencode::TYPE_NONE:
    m_variable = (int64_t)0;
    break;

  case torrent::Bencode::TYPE_VALUE:
    m_variable = arg;
    break;

  case torrent::Bencode::TYPE_STRING:
    first = arg.as_string().c_str();
    value = std::strtoll(first, &last, 0);

    if (last == first || *last != '\0')
      throw torrent::input_error("Could not convert string to value.");

    m_variable = value;
    break;

  default:
    throw torrent::input_error("VariableValue unsupported type restriction.");
  }
}

VariableBool::~VariableBool() {
}

const torrent::Bencode&
VariableBool::get() {
  return m_variable;
}

void
VariableBool::set(const torrent::Bencode& arg) {
  if (arg.is_value()) {
    m_variable = arg.as_value() ? (int64_t)1 : (int64_t)0;

  } else if (arg.is_string()) {

    if (arg.as_string() == "yes" ||
	arg.as_string() == "true")
      m_variable = (int64_t)1;

    else if (arg.as_string() == "no" ||
	     arg.as_string() == "false")
      m_variable = (int64_t)0;

    else
      throw torrent::input_error("String does not parse as a boolean.");

  } else {
    throw torrent::input_error("Input is not a boolean.");
  }
}

VariableBencode::~VariableBencode() {
}

const torrent::Bencode&
VariableBencode::get() {
  if (m_root.empty())
    return m_bencode->get_key(m_key);
  else
    return m_bencode->get_key(m_root).get_key(m_key);
}

void
VariableBencode::set(const torrent::Bencode& arg) {
  // Consider removing if TYPE_NONE.
  torrent::Bencode* root;

  if (m_root.empty())
    root = m_bencode;
  else
    root = &m_bencode->get_key(m_root);

  switch (m_type) {
  case torrent::Bencode::TYPE_NONE:
    root->insert_key(m_key, arg);
    break;

  case torrent::Bencode::TYPE_STRING:
    if (arg.get_type() == torrent::Bencode::TYPE_STRING)
      root->insert_key(m_key, arg);
    else
      throw torrent::input_error("VariableBencode could not convert to string.");
      
    break;

  default:
    throw torrent::input_error("VariableBencode unsupported type restriction.");
  }
}

}
