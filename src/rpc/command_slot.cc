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

#include "rpc/parse.h"

#include "command_slot.h"

namespace utils {

const torrent::Object
CommandSlot::call_unknown(Command* rawCommand, const torrent::Object& rawArgs) {
  CommandSlot* command = static_cast<CommandSlot*>(rawCommand);

  return command->m_slot(rawArgs);
}

const torrent::Object
CommandSlot::call_list(Command* rawCommand, const torrent::Object& rawArgs) {
  CommandSlot* command = static_cast<CommandSlot*>(rawCommand);

  switch (rawArgs.type()) {
  case torrent::Object::TYPE_LIST:
    return command->m_slot(rawArgs);

  case torrent::Object::TYPE_VALUE:
  case torrent::Object::TYPE_STRING:
  {
    torrent::Object tmpList(torrent::Object::TYPE_LIST);
    tmpList.as_list().push_back(rawArgs);

    return command->m_slot(tmpList);
  }
  default:
    throw torrent::input_error("Not a list.");
  }
}

const torrent::Object
CommandSlot::call_value_base(Command* rawCommand, const torrent::Object& rawArgs, int base, int unit) {
  CommandSlot* command = static_cast<CommandSlot*>(rawCommand);

  const torrent::Object& arg = convert_to_single_argument(rawArgs);

  switch (arg.type()) {
  case torrent::Object::TYPE_VALUE:
    // Should shift this one too, so it gives the right unit.
    return command->m_slot(arg);

  case torrent::Object::TYPE_STRING:
  {
    torrent::Object argValue(torrent::Object::TYPE_VALUE);

    if (!utils::parse_whole_value_nothrow(arg.as_string().c_str(), &argValue.as_value(), base, unit))
      throw torrent::input_error("Not a value.");

    return command->m_slot(argValue);
  }
  default:
    throw torrent::input_error("Not a value.");
  }
}

const torrent::Object
CommandSlot::call_string(Command* rawCommand, const torrent::Object& rawArgs) {
  CommandSlot* command = static_cast<CommandSlot*>(rawCommand);

  const torrent::Object& arg = convert_to_single_argument(rawArgs);

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
// CommandSlot::get_generic(Command* rawCommand, const torrent::Object& args) {
//   CommandVariable* variable = static_cast<CommandVariable*>(rawCommand);

//   return variable->m_variable;
// }

}
