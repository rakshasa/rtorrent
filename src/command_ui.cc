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

#include <rak/functional.h>
#include <rak/functional_fun.h>
#include <sigc++/bind.h>

#include "core/manager.h"
#include "core/view_manager.h"
#include "rpc/command_slot.h"
#include "rpc/command_variable.h"
#include "rpc/parse.h"

#include "globals.h"
#include "control.h"
#include "command_helpers.h"

typedef void (core::ViewManager::*view_filter_slot)(const std::string&, const core::ViewManager::sort_args&);

torrent::Object
apply_view_filter(view_filter_slot viewFilterSlot, const torrent::Object& rawArgs) {
  const torrent::Object::list_type& args = rawArgs.as_list();

  if (args.size() < 1)
    throw torrent::input_error("Too few arguments.");

  const std::string& name = args.front().as_string();
  
  if (name.empty())
    throw torrent::input_error("First argument must be a string.");

  core::ViewManager::filter_args filterArgs;

  for (torrent::Object::list_type::const_iterator itr = ++args.begin(), last = args.end(); itr != last; itr++)
    filterArgs.push_back(itr->as_string());

  (control->view_manager()->*viewFilterSlot)(name, filterArgs);

  return torrent::Object();
}

torrent::Object
apply_view_sort(const torrent::Object& rawArgs) {
  const torrent::Object::list_type& args = rawArgs.as_list();

  if (args.size() <= 0 || args.size() > 2)
    throw torrent::input_error("Wrong argument count.");

  const std::string& name = args.front().as_string();

  if (name.empty())
    throw torrent::input_error("First argument must be a string.");

  int32_t value = 0;

  if (args.size() == 2)
    value = rpc::convert_to_value(args.back());

  control->view_manager()->sort(name, value);

  return torrent::Object();
}

torrent::Object
apply_view_list(const torrent::Object&) {
  torrent::Object rawResult(torrent::Object::TYPE_LIST);
  torrent::Object::list_type& result = rawResult.as_list();

  for (core::ViewManager::const_iterator itr = control->view_manager()->begin(), last = control->view_manager()->end(); itr != last; itr++)
    result.push_back((*itr)->name());

  return rawResult;
}

torrent::Object
apply_print(rpc::target_type target, const torrent::Object& rawArgs) {
  char buffer[1024];
  rpc::print_object(buffer, buffer + 1024, &rawArgs, 0);

  control->core()->push_log(buffer);
  return torrent::Object();
}

torrent::Object
apply_cat(rpc::target_type target, const torrent::Object& rawArgs) {
  std::string result;

  rpc::print_object_std(&result, &rawArgs, 0);
  return result;
}

// A series of if/else statements. Every even arguments are
// conditionals and odd arguments are branches to be executed, except
// the last one which is always a branch.
//
// if (cond1) { branch1 }
// <cond1>,<branch1>
//
// if (cond1) { branch1 } else if (cond2) { branch2 } else { branch3 }
// <cond1>,<branch1>,<cond2>,<branch2>,<branch3>
torrent::Object
apply_if(rpc::target_type target, const torrent::Object& rawArgs) {
  const torrent::Object::list_type& args = rawArgs.as_list();
  torrent::Object::list_type::const_iterator itr = args.begin();

  while (itr != args.end() && itr != --args.end()) {
    torrent::Object tmp;
    const torrent::Object* conditional;

    if (itr->is_string() && *itr->as_string().c_str() == '$')
      conditional = &(tmp = rpc::parse_command(target, itr->as_string().c_str() + 1, itr->as_string().c_str() + itr->as_string().size()).first);
    else
      conditional = &*itr;

    bool result;

    switch (conditional->type()) {
    case torrent::Object::TYPE_STRING:
      result = !conditional->as_string().empty();
      break;
    case torrent::Object::TYPE_VALUE:
      result = conditional->as_value();
      break;
    default:
      throw torrent::input_error("Type not supported by 'if'.");
    };

    itr++;

    if (result)
      break;

    itr++;
  }

  if (itr == args.end())
    return torrent::Object();

  if (itr->is_string() && *itr->as_string().c_str() == '$')
    return rpc::parse_command(target, itr->as_string().c_str() + 1, itr->as_string().c_str() + itr->as_string().size()).first;
  else
    return *itr;
}

void
initialize_command_ui() {
  ADD_VARIABLE_STRING("key_layout", "qwerty");

  ADD_COMMAND_STRING("view_add",        rpc::object_string_fn(rak::make_mem_fun(control->view_manager(), &core::ViewManager::insert_throw)));
  ADD_COMMAND_NONE_L("view_list",       rak::ptr_fn(&apply_view_list));

  ADD_COMMAND_LIST("view_filter",       rak::bind_ptr_fn(&apply_view_filter, &core::ViewManager::set_filter));
  ADD_COMMAND_LIST("view_filter_on",    rak::bind_ptr_fn(&apply_view_filter, &core::ViewManager::set_filter_on));

  ADD_COMMAND_LIST("view_sort",         rak::ptr_fn(&apply_view_sort));
  ADD_COMMAND_LIST("view_sort_new",     rak::bind_ptr_fn(&apply_view_filter, &core::ViewManager::set_sort_new));
  ADD_COMMAND_LIST("view_sort_current", rak::bind_ptr_fn(&apply_view_filter, &core::ViewManager::set_sort_current));

//   ADD_COMMAND_LIST("view_sort_current", rak::bind_ptr_fn(&apply_view_filter, &core::ViewManager::set_sort_current));

  ADD_ANY_NONE("print",             rak::ptr_fn(&apply_print));
  ADD_ANY_NONE("cat",               rak::ptr_fn(&apply_cat));
  ADD_ANY_NONE("if",                rak::ptr_fn(&apply_if));
}
