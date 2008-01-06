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

#include "core/download.h"
#include "parse.h"

#include "command_slot.h"

namespace rpc {

template <typename Target> const torrent::Object
CommandSlot<Target>::call_unknown(Command* rawCommand, Target target, const torrent::Object& rawArgs) {
  CommandSlot* command = static_cast<CommandSlot*>(rawCommand);

  return command->m_slot(target, rawArgs);
}

const torrent::Object
CommandSlot<void>::call_unknown(Command* rawCommand, const torrent::Object& rawArgs) {
  CommandSlot* command = static_cast<CommandSlot*>(rawCommand);

  return command->m_slot(rawArgs);
}

template <typename Target> const torrent::Object
CommandSlot<Target>::call_list(Command* rawCommand, Target target, const torrent::Object& rawArgs) {
  CommandSlot* command = static_cast<CommandSlot*>(rawCommand);

  switch (rawArgs.type()) {
  case torrent::Object::TYPE_LIST:
    return command->m_slot(target, rawArgs);

  case torrent::Object::TYPE_VALUE:
  case torrent::Object::TYPE_STRING:
  case torrent::Object::TYPE_NONE:
  {
    torrent::Object tmpList = torrent::Object::create_list();
    tmpList.as_list().push_back(rawArgs);

    return command->m_slot(target, tmpList);
  }
  default:
    throw torrent::input_error("Not a list.");
  }
}

const torrent::Object
CommandSlot<void>::call_list(Command* rawCommand, const torrent::Object& rawArgs) {
  CommandSlot* command = static_cast<CommandSlot*>(rawCommand);

  switch (rawArgs.type()) {
  case torrent::Object::TYPE_LIST:
    return command->m_slot(rawArgs);

  case torrent::Object::TYPE_VALUE:
  case torrent::Object::TYPE_STRING:
  case torrent::Object::TYPE_NONE:
  {
    torrent::Object tmpList = torrent::Object::create_list();
    tmpList.as_list().push_back(rawArgs);

    return command->m_slot(tmpList);
  }
  default:
    throw torrent::input_error("Not a list.");
  }
}

template <typename Target> const torrent::Object
CommandSlot<Target>::call_value_base(Command* rawCommand, Target target, const torrent::Object& rawArgs, int base, int unit) {
  CommandSlot* command = static_cast<CommandSlot*>(rawCommand);

  const torrent::Object& arg = convert_to_single_argument(rawArgs);

  switch (arg.type()) {
  case torrent::Object::TYPE_VALUE:
    // Should shift this one too, so it gives the right unit.
    return command->m_slot(target, arg);

  case torrent::Object::TYPE_STRING:
  {
    torrent::Object argValue = torrent::Object::create_value();

    if (!parse_whole_value_nothrow(arg.as_string().c_str(), &argValue.as_value(), base, unit))
      throw torrent::input_error("Not a value.");

    return command->m_slot(target, argValue);
  }
  default:
    throw torrent::input_error("Not a value.");
  }
}

const torrent::Object
CommandSlot<void>::call_value_base(Command* rawCommand, const torrent::Object& rawArgs, int base, int unit) {
  CommandSlot* command = static_cast<CommandSlot*>(rawCommand);

  const torrent::Object& arg = convert_to_single_argument(rawArgs);

  switch (arg.type()) {
  case torrent::Object::TYPE_VALUE:
    // Should shift this one too, so it gives the right unit.
    return command->m_slot(arg);

  case torrent::Object::TYPE_STRING:
  {
    torrent::Object argValue = torrent::Object::create_value();

    if (!parse_whole_value_nothrow(arg.as_string().c_str(), &argValue.as_value(), base, unit))
      throw torrent::input_error("Not a value.");

    return command->m_slot(argValue);
  }
  default:
    throw torrent::input_error("Not a value.");
  }
}

template <typename Target> const torrent::Object
CommandSlot<Target>::call_string(Command* rawCommand, Target target, const torrent::Object& rawArgs) {
  CommandSlot* command = static_cast<CommandSlot*>(rawCommand);

  const torrent::Object& arg = convert_to_single_argument(rawArgs);

  switch (arg.type()) {
//   case torrent::Object::TYPE_VALUE:
//     break;

  case torrent::Object::TYPE_STRING:
    return command->m_slot(target, arg);
    break;

  default:
    throw torrent::input_error("Not a string.");
  }
}

const torrent::Object
CommandSlot<void>::call_string(Command* rawCommand, const torrent::Object& rawArgs) {
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

torrent::Object
set_variable_d_fn_t::operator () (core::Download* download, const torrent::Object& arg1) {
  if (m_firstKey == NULL)
    download->bencode()->get_key(m_secondKey) = arg1;
  else
    download->bencode()->get_key(m_firstKey).get_key(m_secondKey) = arg1;

  return torrent::Object();
}

torrent::Object
get_variable_d_fn_t::operator () (core::Download* download, const torrent::Object& arg1) {
  if (m_firstKey == NULL)
    return download->bencode()->get_key(m_secondKey);
  else
    return download->bencode()->get_key(m_firstKey).get_key(m_secondKey);
}

// Initialize the necessary template classes.
template class CommandSlot<void>;
template class CommandSlot<target_type>;
template class CommandSlot<core::Download*>;
template class CommandSlot<torrent::File*>;
template class CommandSlot<torrent::FileListIterator*>;
template class CommandSlot<torrent::Peer*>;
template class CommandSlot<torrent::Tracker*>;

}
