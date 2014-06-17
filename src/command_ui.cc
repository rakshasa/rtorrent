// rTorrent - BitTorrent client
// Copyright (C) 2005-2011, Jari Sundell
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

#include <sys/types.h>

#include <ctime>
#include <rak/functional.h>
#include <rak/functional_fun.h>

#include "core/manager.h"
#include "core/view_manager.h"
#include "ui/root.h"
#include "ui/download_list.h"
#include "rpc/parse.h"

#include "globals.h"
#include "control.h"
#include "command_helpers.h"

typedef void (core::ViewManager::*view_event_slot)(const std::string&, const torrent::Object&);

torrent::Object
apply_view_filter_on(const torrent::Object::list_type& args) {
  if (args.size() < 1)
    throw torrent::input_error("Too few arguments.");

  const std::string& name = args.front().as_string();
  
  if (name.empty())
    throw torrent::input_error("First argument must be a string.");

  core::ViewManager::filter_args filterArgs;

  for (torrent::Object::list_const_iterator itr = ++args.begin(), last = args.end(); itr != last; itr++)
    filterArgs.push_back(itr->as_string());

  control->view_manager()->set_filter_on(name, filterArgs);

  return torrent::Object();
}

torrent::Object
apply_view_event(view_event_slot viewFilterSlot, const torrent::Object::list_type& args) {
  if (args.size() != 2)
    throw torrent::input_error("Wrong argument count.");

  (control->view_manager()->*viewFilterSlot)(args.front().as_string(), args.back());
  return torrent::Object();
}

torrent::Object
apply_view_sort(const torrent::Object::list_type& args) {
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
apply_view_list() {
  torrent::Object rawResult = torrent::Object::create_list();
  torrent::Object::list_type& result = rawResult.as_list();

  for (core::ViewManager::const_iterator itr = control->view_manager()->begin(), last = control->view_manager()->end(); itr != last; itr++)
    result.push_back((*itr)->name());

  return rawResult;
}

torrent::Object
apply_view_set(const torrent::Object::list_type& args) {
  if (args.size() != 2)
    throw torrent::input_error("Wrong argument count.");

  core::ViewManager::iterator itr = control->view_manager()->find(args.back().as_string());

  if (itr == control->view_manager()->end())
    throw torrent::input_error("Could not find view \"" + args.back().as_string() + "\".");

//   if (args.front().as_string() == "main")
//     control->ui()->download_list()->set_view(*itr);
//   else
    throw torrent::input_error("No such target.");
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

// Move these boolean operators to a new file.

inline bool
as_boolean(const torrent::Object& rawArgs) {
  switch (rawArgs.type()) {
  case torrent::Object::TYPE_VALUE:  return rawArgs.as_value();
  case torrent::Object::TYPE_STRING: return !rawArgs.as_string().empty();

  // We need to properly handle argument lists that are single-object
  // at a higher level.
  case torrent::Object::TYPE_LIST:   return !rawArgs.as_list().empty() && as_boolean(rawArgs.as_list().front());
  // case torrent::Object::TYPE_MAP:    return !rawArgs.as_map().empty();
  default: return false;
  }
}

torrent::Object
apply_not(rpc::target_type target, const torrent::Object& rawArgs) {
  bool result;

  if (rawArgs.is_dict_key())
    result = as_boolean(rpc::commands.call_command(rawArgs.as_dict_key().c_str(), rawArgs.as_dict_obj(),
                                                   target));
  // TODO: Thus we should clean this up...
  else if (rawArgs.is_list() && !rawArgs.as_list().empty())
    return apply_not(target, rawArgs.as_list().front());

  else
    result = as_boolean(rawArgs);

  return (int64_t)!result;
}

torrent::Object
apply_false(rpc::target_type target, const torrent::Object& rawArgs) {
  return (int64_t)0;
}

torrent::Object
apply_and(rpc::target_type target, const torrent::Object& rawArgs) {
  if (rawArgs.type() != torrent::Object::TYPE_LIST)
    return as_boolean(rawArgs);

  for (torrent::Object::list_const_iterator itr = rawArgs.as_list().begin(), last = rawArgs.as_list().end(); itr != last; itr++)
    if (itr->is_dict_key()) {
      if (!as_boolean(rpc::commands.call_command(itr->as_dict_key().c_str(), itr->as_dict_obj(), target)))
        return (int64_t)false;

    } else if (itr->is_value()) {
      if (!itr->as_value())      
        return (int64_t)false;

    } else {        
      // TODO: Switch to new versions that only accept the new command syntax.
      if (!as_boolean(rpc::parse_command_single(target, itr->as_string())))
        return (int64_t)false;
    }

  return (int64_t)true;
}

torrent::Object
apply_or(rpc::target_type target, const torrent::Object& rawArgs) {
  if (rawArgs.type() != torrent::Object::TYPE_LIST)
    return as_boolean(rawArgs);

  for (torrent::Object::list_const_iterator itr = rawArgs.as_list().begin(), last = rawArgs.as_list().end(); itr != last; itr++)
    if (itr->is_dict_key()) {
      if (as_boolean(rpc::commands.call_command(itr->as_dict_key().c_str(), itr->as_dict_obj(), target)))
        return (int64_t)true;

    } else if (itr->is_value()) {
      if (itr->as_value())      
        return (int64_t)true;

    } else {        
      if (as_boolean(rpc::parse_command_single(target, itr->as_string())))
        return (int64_t)true;
    }

  return (int64_t)false;
}

torrent::Object
apply_cmp(rpc::target_type target, const torrent::Object::list_type& args) {
  // We only need to check if empty() since if size() == 1 it calls
  // the same command for both, or if size() == 2 then each side of
  // the comparison has different commands.
  if (args.empty())
    throw torrent::input_error("Wrong argument count.");

  // This really should be converted to using args flagged as
  // commands, so that we can compare commands and statics values.

  torrent::Object result1;
  torrent::Object result2;

  rpc::target_type target1 = rpc::is_target_pair(target) ? rpc::get_target_left(target) : target;
  rpc::target_type target2 = rpc::is_target_pair(target) ? rpc::get_target_right(target) : target;

  if (args.front().is_dict_key())
    result1 = rpc::commands.call_command(args.front().as_dict_key().c_str(), args.front().as_dict_obj(), target1);
  else
    result1 = rpc::parse_command_single(target1, args.front().as_string());

  if (args.back().is_dict_key())
    result2 = rpc::commands.call_command(args.back().as_dict_key().c_str(), args.back().as_dict_obj(), target2);
  else
    result2 = rpc::parse_command_single(target2, args.back().as_string());

  if (result1.type() != result2.type())
    throw torrent::input_error("Type mismatch.");
    
  switch (result1.type()) {
  case torrent::Object::TYPE_VALUE:  return result1.as_value() - result2.as_value();
  case torrent::Object::TYPE_STRING: return result1.as_string().compare(result2.as_string());
  default: return torrent::Object();
  }
}

torrent::Object apply_less(rpc::target_type target, const torrent::Object::list_type& args) {
  torrent::Object result = apply_cmp(target, args);
  return result.is_value() ? result.as_value() <  0 : (int64_t)false;
}

torrent::Object apply_greater(rpc::target_type target, const torrent::Object::list_type& args) {
  torrent::Object result = apply_cmp(target, args);
  return result.is_value() ? result.as_value() >  0 : (int64_t)false;
}

torrent::Object apply_equal(rpc::target_type target, const torrent::Object::list_type& args) {
  torrent::Object result = apply_cmp(target, args);
  return result.is_value() ? result.as_value() == 0 : (int64_t)false;
}

torrent::Object
apply_to_time(const torrent::Object& rawArgs, int flags) {
  std::tm *u;
  time_t t = (uint64_t)rawArgs.as_value();

  if (flags & 0x1)
    u = std::localtime(&t);
  else
    u = std::gmtime(&t);
  
  if (u == NULL)
    return torrent::Object();

  char buffer[11];

  if (flags & 0x2)
    snprintf(buffer, 11, "%02u/%02u/%04u", u->tm_mday, (u->tm_mon + 1), (1900 + u->tm_year));
  else
    snprintf(buffer, 9, "%2d:%02d:%02d", u->tm_hour, u->tm_min, u->tm_sec);

  return std::string(buffer);
}

torrent::Object
apply_to_elapsed_time(const torrent::Object& rawArgs) {
  uint64_t arg = cachedTime.seconds() - rawArgs.as_value();

  char buffer[48];
  snprintf(buffer, 48, "%2d:%02d:%02d", (int)(arg / 3600), (int)((arg / 60) % 60), (int)(arg % 60));

  return std::string(buffer);
}

torrent::Object
apply_to_kb(const torrent::Object& rawArgs) {
  char buffer[32];
  snprintf(buffer, 32, "%5.1f", (double)rawArgs.as_value() / (1 << 10));

  return std::string(buffer);
}

torrent::Object
apply_to_mb(const torrent::Object& rawArgs) {
  char buffer[32];
  snprintf(buffer, 32, "%8.1f", (double)rawArgs.as_value() / (1 << 20));

  return std::string(buffer);
}

torrent::Object
apply_to_xb(const torrent::Object& rawArgs) {
  char buffer[48];
  int64_t arg = rawArgs.as_value();  

  if (arg < (int64_t(1000) << 10))
    snprintf(buffer, 48, "%5.1f KB", (double)arg / (int64_t(1) << 10));
  else if (arg < (int64_t(1000) << 20))
    snprintf(buffer, 48, "%5.1f MB", (double)arg / (int64_t(1) << 20));
  else if (arg < (int64_t(1000) << 30))
    snprintf(buffer, 48, "%5.1f GB", (double)arg / (int64_t(1) << 30));
  else
    snprintf(buffer, 48, "%5.1f TB", (double)arg / (int64_t(1) << 40));

  return std::string(buffer);
}

torrent::Object
apply_to_throttle(const torrent::Object& rawArgs) {
  int64_t arg = rawArgs.as_value();  
  if (arg < 0)
    return "---";
  else if (arg == 0)
    return "off";

  char buffer[32];
  snprintf(buffer, 32, "%3d", (int)(arg / (1 << 10)));
  return std::string(buffer);
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
apply_if(rpc::target_type target, const torrent::Object& rawArgs, int flags) {
  const torrent::Object::list_type& args = rawArgs.as_list();
  torrent::Object::list_const_iterator itr = args.begin();

  while (itr != args.end() && itr != --args.end()) {
    torrent::Object tmp;
    const torrent::Object* conditional;

    if (flags & 0x1 && itr->is_string())
      conditional = &(tmp = rpc::parse_command(target, itr->as_string().c_str(), itr->as_string().c_str() + itr->as_string().size()).first);
    else if (flags & 0x1 && itr->is_dict_key())
      conditional = &(tmp = rpc::commands.call_command(itr->as_dict_key().c_str(), itr->as_dict_obj(), target));
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
    case torrent::Object::TYPE_NONE:
      result = false;
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

  if (flags & 0x1 && itr->is_string()) {
    return rpc::parse_command(target, itr->as_string().c_str(), itr->as_string().c_str() + itr->as_string().size()).first;

  } else if (flags & 0x1 && itr->is_dict_key()) {
    return rpc::commands.call_command(itr->as_dict_key().c_str(), itr->as_dict_obj(), target);

  } else if (flags & 0x1 && itr->is_list()) {
    // Move this into a special function or something. Also, might be
    // nice to have a parse_command function that takes list
    // iterator...

    for (torrent::Object::list_type::const_iterator cmdItr = itr->as_list().begin(), last = itr->as_list().end(); cmdItr != last; cmdItr++)
      if (cmdItr->is_string())
        rpc::parse_command(target, cmdItr->as_string().c_str(), cmdItr->as_string().c_str() + cmdItr->as_string().size());

    return torrent::Object();

  } else {
    return *itr;
  }
}

torrent::Object
cmd_view_size(const torrent::Object::string_type& args) {
  return (*control->view_manager()->find_throw(args))->size_visible();
}

torrent::Object
cmd_view_size_not_visible(const torrent::Object::string_type& args) {
  return (*control->view_manager()->find_throw(args))->size_not_visible();
}

torrent::Object
cmd_view_persistent(const torrent::Object::string_type& args) {
  core::View* view = *control->view_manager()->find_throw(args);
  
  if (!view->get_filter().is_empty() || !view->event_added().is_empty() || !view->event_removed().is_empty())
    throw torrent::input_error("Cannot set modified views as persitent.");

  view->set_filter("d.views.has=" + args);
  view->set_event_added("d.views.push_back_unique=" + args);
  view->set_event_removed("d.views.remove=" + args);

  return torrent::Object();
}

// TODO: These don't need wrapper functions anymore...
torrent::Object
cmd_ui_set_view(const torrent::Object::string_type& args) {
  control->ui()->download_list()->set_current_view(args);
  return torrent::Object();
}

torrent::Object
cmd_ui_unfocus_download(core::Download* download) {
  control->ui()->download_list()->unfocus_download(download);

  return torrent::Object();
}

torrent::Object
cmd_view_filter_download(core::Download* download, const torrent::Object::string_type& args) {
  (*control->view_manager()->find_throw(args))->filter_download(download);

  return torrent::Object();
}

torrent::Object
cmd_view_set_visible(core::Download* download, const torrent::Object::string_type& args) {
  (*control->view_manager()->find_throw(args))->set_visible(download);

  return torrent::Object();
}

torrent::Object
cmd_view_set_not_visible(core::Download* download, const torrent::Object::string_type& args) {
  (*control->view_manager()->find_throw(args))->set_not_visible(download);

  return torrent::Object();
}

torrent::Object
apply_elapsed_less(const torrent::Object::list_type& args) {
  if (args.size() != 2)
    throw torrent::input_error("Wrong argument count.");

  int64_t start_time = rpc::convert_to_value(args.front());

  return (int64_t)(start_time != 0 && rak::timer::current_seconds() - start_time < rpc::convert_to_value(args.back()));
}

torrent::Object
apply_elapsed_greater(const torrent::Object::list_type& args) {
  if (args.size() != 2)
    throw torrent::input_error("Wrong argument count.");

  int64_t start_time = rpc::convert_to_value(args.front());

  return (int64_t)(start_time != 0 && rak::timer::current_seconds() - start_time > rpc::convert_to_value(args.back()));
}

void
initialize_command_ui() {
  CMD2_VAR_STRING("keys.layout", "qwerty");

  CMD2_ANY_STRING("view.add", object_convert_void(std::bind(&core::ViewManager::insert_throw, control->view_manager(), std::placeholders::_2)));

  CMD2_ANY_L   ("view.list",          std::bind(&apply_view_list));
  CMD2_ANY_LIST("view.set",           std::bind(&apply_view_set, std::placeholders::_2));

  CMD2_ANY_LIST("view.filter",        std::bind(&apply_view_event, &core::ViewManager::set_filter, std::placeholders::_2));
  CMD2_ANY_LIST("view.filter_on",     std::bind(&apply_view_filter_on, std::placeholders::_2));

  CMD2_ANY_LIST("view.sort",          std::bind(&apply_view_sort, std::placeholders::_2));
  CMD2_ANY_LIST("view.sort_new",      std::bind(&apply_view_event, &core::ViewManager::set_sort_new, std::placeholders::_2));
  CMD2_ANY_LIST("view.sort_current",  std::bind(&apply_view_event, &core::ViewManager::set_sort_current, std::placeholders::_2));

  CMD2_ANY_LIST("view.event_added",   std::bind(&apply_view_event, &core::ViewManager::set_event_added, std::placeholders::_2));
  CMD2_ANY_LIST("view.event_removed", std::bind(&apply_view_event, &core::ViewManager::set_event_removed, std::placeholders::_2));

  // Cleanup and add . to view.

  CMD2_ANY_STRING("view.size",              std::bind(&cmd_view_size, std::placeholders::_2));
  CMD2_ANY_STRING("view.size_not_visible",  std::bind(&cmd_view_size_not_visible, std::placeholders::_2));
  CMD2_ANY_STRING("view.persistent",        std::bind(&cmd_view_persistent, std::placeholders::_2));

  CMD2_ANY_STRING_V("view.filter_all",      std::bind(&core::View::filter, std::bind(&core::ViewManager::find_ptr_throw, control->view_manager(), std::placeholders::_2)));

  CMD2_DL_STRING ("view.filter_download", std::bind(&cmd_view_filter_download, std::placeholders::_1, std::placeholders::_2));
  CMD2_DL_STRING ("view.set_visible",     std::bind(&cmd_view_set_visible,     std::placeholders::_1, std::placeholders::_2));
  CMD2_DL_STRING ("view.set_not_visible", std::bind(&cmd_view_set_not_visible, std::placeholders::_1, std::placeholders::_2));

  // Commands that affect the default rtorrent UI.
  CMD2_DL        ("ui.unfocus_download",   std::bind(&cmd_ui_unfocus_download, std::placeholders::_1));
  CMD2_ANY_STRING("ui.current_view.set",   std::bind(&cmd_ui_set_view, std::placeholders::_2));

  // Move.
  CMD2_ANY("print", &apply_print);
  CMD2_ANY("cat",   &apply_cat);
  CMD2_ANY("if",    std::bind(&apply_if, std::placeholders::_1, std::placeholders::_2, 0));
  CMD2_ANY("not",   &apply_not);
  CMD2_ANY("false", &apply_false);
  CMD2_ANY("and",   &apply_and);
  CMD2_ANY("or",    &apply_or);

  // A temporary command for handling stuff until we get proper
  // support for seperation of commands and literals.
  CMD2_ANY("branch", std::bind(&apply_if, std::placeholders::_1, std::placeholders::_2, 1));

  CMD2_ANY_LIST("less",    &apply_less);
  CMD2_ANY_LIST("greater", &apply_greater);
  CMD2_ANY_LIST("equal",   &apply_equal);

  CMD2_ANY_VALUE("convert.gm_time",      std::bind(&apply_to_time, std::placeholders::_2, 0));
  CMD2_ANY_VALUE("convert.gm_date",      std::bind(&apply_to_time, std::placeholders::_2, 0x2));
  CMD2_ANY_VALUE("convert.time",         std::bind(&apply_to_time, std::placeholders::_2, 0x1));
  CMD2_ANY_VALUE("convert.date",         std::bind(&apply_to_time, std::placeholders::_2, 0x1 | 0x2));
  CMD2_ANY_VALUE("convert.elapsed_time", std::bind(&apply_to_elapsed_time, std::placeholders::_2));
  CMD2_ANY_VALUE("convert.kb",           std::bind(&apply_to_kb, std::placeholders::_2));
  CMD2_ANY_VALUE("convert.mb",           std::bind(&apply_to_mb, std::placeholders::_2));
  CMD2_ANY_VALUE("convert.xb",           std::bind(&apply_to_xb, std::placeholders::_2));
  CMD2_ANY_VALUE("convert.throttle",     std::bind(&apply_to_throttle, std::placeholders::_2));

  CMD2_ANY_LIST ("elapsed.less",         std::bind(&apply_elapsed_less, std::placeholders::_2));
  CMD2_ANY_LIST ("elapsed.greater",      std::bind(&apply_elapsed_greater, std::placeholders::_2));
}
