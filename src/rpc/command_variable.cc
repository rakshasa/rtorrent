// rTorrent - BitTorrent client
// Copyright (C) 2005-2007, Jari Sundell
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

#include "parse.h"
#include "command_variable.h"

namespace rpc {

const torrent::Object
CommandVariable::set_bool(Command* rawCommand, const torrent::Object& rawArgs) {
  CommandVariable* variable = static_cast<CommandVariable*>(rawCommand);

  const torrent::Object& arg = convert_to_single_argument(rawArgs);

  switch (arg.type()) {
  case torrent::Object::TYPE_VALUE:
    variable->m_variable = arg.as_value() ? (int64_t)1 : (int64_t)0;
    break;

  case torrent::Object::TYPE_STRING:
    // Move the checks into some is_true, is_false think in Command.
    if (arg.as_string() == "yes" || arg.as_string() == "true")
      variable->m_variable = (int64_t)1;

    else if (arg.as_string() == "no" || arg.as_string() == "false")
      variable->m_variable = (int64_t)0;

    else
      throw torrent::input_error("String does not parse as a boolean.");

    break;

  default:
    throw torrent::input_error("Input is not a boolean.");
  }

  return variable->m_variable;
}

const torrent::Object
CommandVariable::get_bool(Command* rawCommand, const torrent::Object& args) {
  CommandVariable* variable = static_cast<CommandVariable*>(rawCommand);

  return variable->m_variable;
}

const torrent::Object
CommandVariable::set_value(Command* rawCommand, const torrent::Object& rawArgs) {
  CommandVariable* variable = static_cast<CommandVariable*>(rawCommand);

  const torrent::Object& arg = convert_to_single_argument(rawArgs);

  switch (arg.type()) {
  case torrent::Object::TYPE_NONE:
    variable->m_variable = (int64_t)0;
    break;

  case torrent::Object::TYPE_VALUE:
    variable->m_variable = arg;
    break;

  case torrent::Object::TYPE_STRING:
    int64_t value;
    parse_whole_value(arg.as_string().c_str(), &value, 0, 1);

    variable->m_variable = value;
    break;

  default:
    throw torrent::input_error("CommandValue unsupported type restriction.");
  }

  return variable->m_variable;
}

const torrent::Object
CommandVariable::get_value(Command* rawCommand, const torrent::Object& args) {
  CommandVariable* variable = static_cast<CommandVariable*>(rawCommand);

  return variable->m_variable;
}

const torrent::Object
CommandVariable::set_string(Command* rawCommand, const torrent::Object& rawArgs) {
  CommandVariable* variable = static_cast<CommandVariable*>(rawCommand);

  const torrent::Object& arg = convert_to_single_argument(rawArgs);

  switch (arg.type()) {
  case torrent::Object::TYPE_NONE:
    variable->m_variable = std::string("");
    break;

//   case torrent::Object::TYPE_VALUE:
//     variable->m_variable = arg;
//     break;

  case torrent::Object::TYPE_STRING:
    variable->m_variable = arg;
    break;

  default:
    throw torrent::input_error("Not a string.");
  }

  return variable->m_variable;
}

const torrent::Object
CommandVariable::get_string(Command* rawCommand, const torrent::Object& args) {
  CommandVariable* variable = static_cast<CommandVariable*>(rawCommand);

  return variable->m_variable;
}

}
