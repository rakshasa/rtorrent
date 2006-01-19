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

#include "variable_generic.h"

namespace utils {

VariableValue::~VariableValue() {
}

const torrent::Bencode&
VariableValue::get() {
  return m_variable;
}

void
VariableValue::set(const torrent::Bencode& arg) {
  m_variable = arg;
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
  return m_bencode->get_key(m_key);
}

void
VariableBencode::set(const torrent::Bencode& arg) {
  // Consider removing if TYPE_NONE.

  switch (m_type) {
  case torrent::Bencode::TYPE_NONE:
    m_bencode->insert_key(m_key, arg);
    break;

  case torrent::Bencode::TYPE_STRING:
    if (arg.get_type() == torrent::Bencode::TYPE_STRING)
      m_bencode->insert_key(m_key, arg);
    else
      throw torrent::input_error("VariableBencode could not convert to string.");
      
    break;

  default:
    throw torrent::input_error("VariableBencode unsupported type restriction.");
  }
}

}
