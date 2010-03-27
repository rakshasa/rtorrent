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

#include <algorithm>
#include <functional>
#include <rak/functional.h>

#include "parse.h"
#include "parse_commands.h"
#include "command_function.h"

namespace rpc {

// Temp until it can be moved somewhere better...
const torrent::Object
command_function_call(const torrent::raw_string& cmd, target_type target, const torrent::Object& args) {
  char* buffer[sizeof(torrent::Object) * Command::max_arguments];
  torrent::Object* stack = (torrent::Object*)buffer;
  torrent::Object* first = (torrent::Object*)buffer;

  if (args.is_list()) {
    // Do nothing for now.
    for (torrent::Object::list_const_iterator itr = args.as_list().begin(), last = args.as_list().end();
         itr != last && first != stack + Command::max_arguments;
         itr++, first++) {
      new (first) torrent::Object(*itr);
      first->swap(*Command::argument(std::distance(stack, first)));
    }

  } else if (args.type() != torrent::Object::TYPE_NONE) {
    new (first) torrent::Object(args);
    (first++)->swap(*Command::argument(0));
  }

  try {
    torrent::Object result = parse_command_multiple(target, cmd.begin(), cmd.end());

    while (first-- != stack) {
      first->swap(*Command::argument(std::distance(stack, first)));
      first->~Object();
    }

    return result;

  } catch (torrent::bencode_error& e) {
    while (first-- != stack) {
      first->swap(*Command::argument(std::distance(stack, first)));
      first->~Object();
    }

    throw e;
  }
}

const torrent::Object
CommandFunction::call(Command* rawCommand, target_type target, const torrent::Object& args) {
  char* buffer[sizeof(torrent::Object) * Command::max_arguments];
  torrent::Object* stack = (torrent::Object*)buffer;
  torrent::Object* first = (torrent::Object*)buffer;

  if (args.is_list()) {
    // Do nothing for now.
    for (torrent::Object::list_const_iterator itr = args.as_list().begin(), last = args.as_list().end();
         itr != last && first != stack + Command::max_arguments;
         itr++, first++) {
      new (first) torrent::Object(*itr);
      first->swap(*argument(std::distance(stack, first)));
    }

  } else if (args.type() != torrent::Object::TYPE_NONE) {
    new (first) torrent::Object(args);
    (first++)->swap(*argument(0));
  }

  CommandFunction* command = reinterpret_cast<CommandFunction*>(rawCommand);

  try {
    torrent::Object result = parse_command_multiple(target, command->m_command.c_str(), command->m_command.c_str() + command->m_command.size());

    while (first-- != stack) {
      first->swap(*argument(std::distance(stack, first)));
      first->~Object();
    }

    return result;

  } catch (torrent::bencode_error& e) {
    while (first-- != stack) {
      first->swap(*Command::argument(std::distance(stack, first)));
      first->~Object();
    }

    throw e;
  }
}

const torrent::Object
CommandFunction::call_redirect(Command* rawCommand, target_type target, const torrent::Object& args) {
  CommandFunction* command = reinterpret_cast<CommandFunction*>(rawCommand);
  
  return commands.call_command(command->m_command.c_str(), args, target);
}

const torrent::Object
CommandFunctionList::call(Command* rawCommand, target_type target, const torrent::Object& args) {
  char* buffer[sizeof(torrent::Object) * Command::max_arguments];
  torrent::Object* stack = (torrent::Object*)buffer;
  torrent::Object* first = (torrent::Object*)buffer;

  if (args.is_list()) {
    // Do nothing for now.
    for (torrent::Object::list_const_iterator itr = args.as_list().begin(), last = args.as_list().end();
         itr != last && first != stack + Command::max_arguments;
         itr++, first++) {
      new (first) torrent::Object(*itr);
      first->swap(*argument(std::distance(stack, first)));
    }

  } else if (args.type() != torrent::Object::TYPE_NONE) {
    new (first) torrent::Object(args);
    (first++)->swap(*argument(0));
  }

  CommandFunctionList* command = reinterpret_cast<CommandFunctionList*>(rawCommand);

  try {
    for (base_type::const_iterator itr = command->begin(), last = command->end(); itr != last; itr++)
      parse_command_multiple(target, itr->second.c_str(), itr->second.c_str() + itr->second.size());

  } catch (torrent::bencode_error& e) {
    while (first-- != stack) {
      first->swap(*Command::argument(std::distance(stack, first)));
      first->~Object();
    }

    throw e;
  }

  while (first-- != stack) {
    first->swap(*argument(std::distance(stack, first)));
    first->~Object();
  }

  return torrent::Object();
}

CommandFunctionList::const_iterator
CommandFunctionList::find(const char* key) {
  base_type::iterator itr = std::find_if(begin(), end(), rak::less_equal(key, rak::mem_ref(&base_type::value_type::first)));

  if (itr == end() || itr->first != key)
    return end();
  else
    return itr;
}

void
CommandFunctionList::insert(const std::string& key, const std::string& cmd) {
  base_type::iterator itr = std::find_if(begin(), end(), rak::less_equal(key, rak::mem_ref(&base_type::value_type::first)));

  if (itr != end() && itr->first == key)
    itr->second = cmd;
  else
    base_type::insert(itr, base_type::value_type(key, cmd));
}

void
CommandFunctionList::erase(const std::string& key) {
  base_type::iterator itr = std::find_if(begin(), end(), rak::less_equal(key, rak::mem_ref(&base_type::value_type::first)));

  if (itr != end() && itr->first == key)
    base_type::erase(itr);
}

}
