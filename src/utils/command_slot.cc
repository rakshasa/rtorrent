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

#include "command_slot.h"

namespace utils {

const torrent::Object
CommandSlot::call_unknown(Variable* rawVariable, const torrent::Object& rawArgs) {
  CommandSlot* command = static_cast<CommandSlot*>(rawVariable);

  return command->m_slot(rawArgs);
}

const torrent::Object
CommandSlot::call_list(Variable* rawVariable, const torrent::Object& rawArgs) {
  CommandSlot* command = static_cast<CommandSlot*>(rawVariable);

//   const torrent::Object& arg = to_single_argument(rawArgs);

//   switch (arg.type()) {
//   case torrent::Object::TYPE_VALUE:
//     variable->m_variable = arg.as_value() ? (int64_t)1 : (int64_t)0;
//     break;

//   case torrent::Object::TYPE_STRING:
//     // Move the checks into some is_true, is_false think in Variable.
//     if (arg.as_string() == "yes" || arg.as_string() == "true")
//       variable->m_variable = (int64_t)1;

//     else if (arg.as_string() == "no" || arg.as_string() == "false")
//       variable->m_variable = (int64_t)0;

//     else
//       throw torrent::input_error("String does not parse as a boolean.");

//     break;

//   default:
//     throw torrent::input_error("Input is not a boolean.");
//   }

//   return variable->m_variable;

  switch (rawArgs.type()) {
//   case torrent::Object::TYPE_STRING:
//     break;
  case torrent::Object::TYPE_LIST:
    return command->m_slot(rawArgs);
  default:
    throw torrent::input_error("Not a list.");
  }
}

const torrent::Object
CommandSlot::call_string(Variable* rawVariable, const torrent::Object& rawArgs) {
  CommandSlot* command = static_cast<CommandSlot*>(rawVariable);

  const torrent::Object& arg = to_single_argument(rawArgs);

  switch (arg.type()) {
//   case torrent::Object::TYPE_VALUE:
//     break;

  case torrent::Object::TYPE_STRING:
    return command->m_slot(arg);
    break;

  default:
    throw torrent::input_error("Not a string.");
  }
}

// const torrent::Object&
// CommandSlot::get_generic(Variable* rawVariable, const torrent::Object& args) {
//   CommandVariable* variable = static_cast<CommandVariable*>(rawVariable);

//   return variable->m_variable;
// }

}
