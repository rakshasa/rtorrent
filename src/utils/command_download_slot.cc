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

#include "core/download.h"
#include "utils/parse.h"

#include "command_download_slot.h"

namespace utils {

const torrent::Object
CommandDownloadSlot::call_unknown(Variable* rawVariable, core::Download* download, const torrent::Object& rawArgs) {
  CommandDownloadSlot* command = static_cast<CommandDownloadSlot*>(rawVariable);

  return command->m_slot(download, rawArgs);
}

const torrent::Object
CommandDownloadSlot::call_list(Variable* rawVariable, core::Download* download, const torrent::Object& rawArgs) {
  CommandDownloadSlot* command = static_cast<CommandDownloadSlot*>(rawVariable);

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
    return command->m_slot(download, rawArgs);
  default:
    throw torrent::input_error("Not a list.");
  }
}

const torrent::Object
CommandDownloadSlot::call_value_base(Variable* rawVariable, core::Download* download, const torrent::Object& rawArgs, int base, int unit) {
  CommandDownloadSlot* command = static_cast<CommandDownloadSlot*>(rawVariable);

  const torrent::Object& arg = to_single_argument(rawArgs);

  switch (arg.type()) {
  case torrent::Object::TYPE_VALUE:
    // Should shift this one too, so it gives the right unit.
    return command->m_slot(download, arg);

  case torrent::Object::TYPE_STRING:
  {
    torrent::Object argValue(torrent::Object::TYPE_VALUE);

    if (!utils::parse_whole_value_nothrow(arg.as_string().c_str(), &argValue.as_value(), base, unit))
      throw torrent::input_error("Not a value.");

    return command->m_slot(download, argValue);
  }
  default:
    throw torrent::input_error("Not a value.");
  }
}

const torrent::Object
CommandDownloadSlot::call_string(Variable* rawVariable, core::Download* download, const torrent::Object& rawArgs) {
  CommandDownloadSlot* command = static_cast<CommandDownloadSlot*>(rawVariable);

  const torrent::Object& arg = to_single_argument(rawArgs);

  switch (arg.type()) {
//   case torrent::Object::TYPE_VALUE:
//     break;

  case torrent::Object::TYPE_STRING:
    return command->m_slot(download, arg);
    break;

  default:
    throw torrent::input_error("Not a string.");
  }
}

// const torrent::Object&
// CommandDownloadSlot::get_generic(Variable* rawVariable, const torrent::Object& args) {
//   CommandVariable* variable = static_cast<CommandVariable*>(rawVariable);

//   return variable->m_variable;
// }

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

}
