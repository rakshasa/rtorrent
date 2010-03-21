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

#include "command_new_slot.h"

namespace rpc {

const torrent::Object
command_base_call_any(Command* rawCommand, target_type target, const torrent::Object& args) {
  return static_cast<command_base*>(rawCommand)->_pod<any_function>()(target, args);
}

const torrent::Object
command_base_call_any_value_base(Command* rawCommand, target_type target, const torrent::Object& rawArgs, int base, int unit) {
  const torrent::Object& arg = convert_to_single_argument(rawArgs);

  if (arg.type() == torrent::Object::TYPE_STRING) {
    torrent::Object::value_type val;

    if (!parse_whole_value_nothrow(arg.as_string().c_str(), &val, base, unit))
      throw torrent::input_error("Not a value.");

    return static_cast<command_base*>(rawCommand)->_pod<any_value_function>()(target, val);
  }

  return static_cast<command_base*>(rawCommand)->_pod<any_value_function>()(target, arg.as_value());
}

const torrent::Object
command_base_call_any_value(Command* rawCommand, target_type target, const torrent::Object& rawArgs) {
  return command_base_call_any_value_base(rawCommand, target, rawArgs, 0, 1);
}


const torrent::Object
command_base_call_any_string(Command* rawCommand, target_type target, const torrent::Object& rawArgs) {
  const torrent::Object& arg = convert_to_single_argument(rawArgs);

  if (arg.type() == torrent::Object::TYPE_RAW_STRING)
    return static_cast<command_base*>(rawCommand)->_pod<any_string_function>()(target, arg.as_raw_string().as_string());

  return static_cast<command_base*>(rawCommand)->_pod<any_string_function>()(target, arg.as_string());
}

const torrent::Object
command_base_call_any_list(Command* rawCommand, target_type target, const torrent::Object& rawArgs) {
  if (rawArgs.type() != torrent::Object::TYPE_LIST) {
    torrent::Object::list_type arg;
    
    if (!rawArgs.is_empty())
      arg.push_back(rawArgs);

    return static_cast<command_base*>(rawCommand)->_pod<any_list_function>()(target, arg);
  }

  return static_cast<command_base*>(rawCommand)->_pod<any_list_function>()(target, rawArgs.as_list());
}

}
