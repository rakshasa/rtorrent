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

#include <functional>
#include <cstdio>
#include <rak/file_stat.h>
#include <rak/path.h>
#include <rak/string_manip.h>
#include <sigc++/adaptors/bind.h>
#include <torrent/rate.h>
#include <torrent/hash_string.h>

#include "core/download.h"
#include "core/download_list.h"
#include "core/manager.h"
#include "core/view_manager.h"
#include "rpc/command_scheduler.h"
#include "rpc/command_slot.h"
#include "rpc/command_variable.h"
#include "rpc/parse.h"
#include "rpc/parse_commands.h"

#include "globals.h"
#include "control.h"
#include "command_helpers.h"

torrent::Object
apply_on_state_change(const char* name, const torrent::Object& rawArgs) {
  const torrent::Object::list_type& args = rawArgs.as_list();

  if (args.size() == 0 || args.size() > 2)
    throw torrent::input_error("Wrong number of arguments.");

  const std::string& rawKey = args.front().as_string();

  if (rawKey.empty())
    throw torrent::input_error("Empty key.");

  // If the key starts with '_' then it's supposed to be literal, with
  // the initial '_' removed. This allows us to get proper ordering
  // for internal rtorrent tasks.
  std::string key = rawKey[0] != '_' ? ("1_state_" + rawKey) : rawKey.substr(1);

  if (args.size() == 1)
    rpc::commands.call("system.method.set_key", rpc::make_target(), rpc::create_object_list(name, key));
  else
    rpc::commands.call("system.method.set_key", rpc::make_target(), rpc::create_object_list(name, key, args.back()));

  // Deprecated notice, remove this function in the next minor
  // version.
  static bool notify = true;

  if (notify) {
    control->core()->push_log("Deprecated on_* commands, use 'system.method.set_key = event.download.{inserted, erased, ...}, <key>, <command>' instead.");
    notify = false;
  }

  return torrent::Object();
}

torrent::Object
apply_on_ratio(const torrent::Object& rawArgs) {
  const std::string& groupName = rawArgs.as_string();

  char buffer[32 + groupName.size()];
  sprintf(buffer, "group.%s.view", groupName.c_str());

  core::ViewManager::iterator viewItr = control->view_manager()->find(rpc::commands.call(buffer, rpc::make_target()).as_string());

  if (viewItr == control->view_manager()->end())
    throw torrent::input_error("Could not find view.");

  char* bufferStart = buffer + sprintf(buffer, "group.%s.ratio.", groupName.c_str());

  // first argument:  minimum ratio to reach
  // second argument: minimum upload amount to reach [optional]
  // third argument:  maximum ratio to reach [optional]
  std::strcpy(bufferStart, "min");
  int64_t minRatio  = rpc::commands.call(buffer, rpc::make_target()).as_value();
  std::strcpy(bufferStart, "max");
  int64_t maxRatio  = rpc::commands.call(buffer, rpc::make_target()).as_value();
  std::strcpy(bufferStart, "upload");
  int64_t minUpload = rpc::commands.call(buffer, rpc::make_target()).as_value();

  std::vector<core::Download*> downloads;

  for  (core::View::iterator itr = (*viewItr)->begin_visible(), last = (*viewItr)->end_visible(); itr != last; itr++) {
    if (!(*itr)->is_seeding() || rpc::call_command_value("d.get_ignore_commands", rpc::make_target(*itr)) != 0)
      continue;

    //    rpc::parse_command_single(rpc::make_target(*itr), "print={Checked ratio of download.}");

    int64_t totalDone   = (*itr)->download()->bytes_done();
    int64_t totalUpload = (*itr)->download()->up_rate()->total();

    if (!(totalUpload >= minUpload && totalUpload * 100 >= totalDone * minRatio) &&
        !(maxRatio > 0 && totalUpload * 100 > totalDone * maxRatio))
      continue;

    downloads.push_back(*itr);
  }

  std::strcpy(bufferStart, "command");

  for (std::vector<core::Download*>::iterator itr = downloads.begin(), last = downloads.end(); itr != last; itr++) {
    //    rpc::commands.call("print", rpc::make_target(*itr), "Calling ratio command.");
    rpc::commands.call_catch(buffer, rpc::make_target(*itr), torrent::Object(), "Ratio reached, but command failed: ");
  }

  return torrent::Object();
}

torrent::Object
apply_start_tied() {
  for (core::DownloadList::iterator itr = control->core()->download_list()->begin(); itr != control->core()->download_list()->end(); ++itr) {
    if (rpc::call_command_value("d.get_state", rpc::make_target(*itr)) == 1)
      continue;

    rak::file_stat fs;
    const std::string& tiedToFile = rpc::call_command_string("d.get_tied_to_file", rpc::make_target(*itr));

    if (!tiedToFile.empty() && fs.update(rak::path_expand(tiedToFile)))
      rpc::parse_command_single(rpc::make_target(*itr), "d.try_start=");
  }

  return torrent::Object();
}

torrent::Object
apply_stop_untied() {
  for (core::DownloadList::iterator itr = control->core()->download_list()->begin(); itr != control->core()->download_list()->end(); ++itr) {
    if (rpc::call_command_value("d.get_state", rpc::make_target(*itr)) == 0)
      continue;

    rak::file_stat fs;
    const std::string& tiedToFile = rpc::call_command_string("d.get_tied_to_file", rpc::make_target(*itr));

    if (!tiedToFile.empty() && !fs.update(rak::path_expand(tiedToFile)))
      rpc::parse_command_single(rpc::make_target(*itr), "d.try_stop=");
  }

  return torrent::Object();
}

torrent::Object
apply_close_untied() {
  for (core::DownloadList::iterator itr = control->core()->download_list()->begin(); itr != control->core()->download_list()->end(); ++itr) {
    rak::file_stat fs;
    const std::string& tiedToFile = rpc::call_command_string("d.get_tied_to_file", rpc::make_target(*itr));

    if (rpc::call_command_value("d.get_ignore_commands", rpc::make_target(*itr)) == 0 && !tiedToFile.empty() && !fs.update(rak::path_expand(tiedToFile)))
      rpc::parse_command_single(rpc::make_target(*itr), "d.try_close=");
  }

  return torrent::Object();
}

torrent::Object
apply_remove_untied() {
  for (core::DownloadList::iterator itr = control->core()->download_list()->begin(); itr != control->core()->download_list()->end(); ) {
    rak::file_stat fs;
    const std::string& tiedToFile = rpc::call_command_string("d.get_tied_to_file", rpc::make_target(*itr));

    if (!tiedToFile.empty() && !fs.update(rak::path_expand(tiedToFile))) {
      // Need to clear tied_to_file so it doesn't try to delete it.
      rpc::call_command("d.set_tied_to_file", std::string(), rpc::make_target(*itr));

      itr = control->core()->download_list()->erase(itr);

    } else {
      ++itr;
    }
  }

  return torrent::Object();
}

torrent::Object
apply_schedule(const torrent::Object& rawArgs) {
  const torrent::Object::list_type& args = rawArgs.as_list();

  if (args.size() != 4)
    throw torrent::input_error("Wrong number of arguments.");

  torrent::Object::list_const_iterator itr = args.begin();

  const std::string& arg1 = (itr++)->as_string();
  const std::string& arg2 = (itr++)->as_string();
  const std::string& arg3 = (itr++)->as_string();
  const std::string& arg4 = (itr++)->as_string();

  control->command_scheduler()->parse(arg1, arg2, arg3, arg4);

  return torrent::Object();
}

torrent::Object
apply_load(int flags, const torrent::Object& rawArgs) {
  const torrent::Object::list_type&    args    = rawArgs.as_list();
  torrent::Object::list_const_iterator argsItr = args.begin();

  if (argsItr == args.end())
    throw torrent::input_error("Too few arguments.");

  const std::string& filename = argsItr->as_string();
  core::Manager::command_list_type commands;

  while (++argsItr != args.end())
    commands.push_back(argsItr->as_string());

  control->core()->try_create_download_expand(filename, flags, commands);

  return torrent::Object();
}

void apply_import(const std::string& path)     { if (!rpc::parse_command_file(path)) throw torrent::input_error("Could not open option file: " + path); }
void apply_try_import(const std::string& path) { if (!rpc::parse_command_file(path)) control->core()->push_log_std("Could not read resource file: " + path); }

void
apply_close_low_diskspace(int64_t arg) {
  core::DownloadList* downloadList = control->core()->download_list();

  bool closed = false;
  core::Manager::DListItr itr = downloadList->begin();

  while ((itr = std::find_if(itr, downloadList->end(), std::mem_fun(&core::Download::is_downloading)))
         != downloadList->end()) {
    if ((*itr)->file_list()->free_diskspace() < (uint64_t)arg) {
      downloadList->close(*itr);

      (*itr)->set_hash_failed(true);
      (*itr)->set_message(std::string("Low diskspace."));

      closed = true;
    }

    ++itr;
  }

  if (closed)
    control->core()->push_log("Closed torrents due to low diskspace.");    
}

torrent::Object
apply_download_list(const torrent::Object& rawArgs) {
  const torrent::Object::list_type&          args    = rawArgs.as_list();
  torrent::Object::list_const_iterator argsItr = args.begin();

  core::ViewManager* viewManager = control->view_manager();
  core::ViewManager::iterator viewItr;

  if (argsItr != args.end() && !argsItr->as_string().empty())
    viewItr = viewManager->find((argsItr++)->as_string());
  else
    viewItr = viewManager->find("default");

  if (viewItr == viewManager->end())
    throw torrent::input_error("Could not find view.");

  torrent::Object result = torrent::Object::create_list();
  torrent::Object::list_type& resultList = result.as_list();

  for (core::View::const_iterator itr = (*viewItr)->begin_visible(), last = (*viewItr)->end_visible(); itr != last; itr++) {
    const torrent::HashString* hashString = &(*itr)->download()->info_hash();

    resultList.push_back(rak::transform_hex(hashString->begin(), hashString->end()));
  }

  return result;
}

torrent::Object
d_multicall(const torrent::Object& rawArgs) {
  const torrent::Object::list_type&          args = rawArgs.as_list();

  if (args.empty())
    throw torrent::input_error("Too few arguments.");

//   const torrent::Object::string_type& infoHash = args.begin()->as_string();

//   core::DownloadList*          dList = control->core()->download_list();
//   core::DownloadList::iterator dItr  = dList->end();

//   if (infoHash.size() == 40)
//     dItr = dList->find_hex(infoHash.c_str());

//   if (dItr == dList->end())
//     throw torrent::input_error("Not a valid info-hash.");

  core::ViewManager* viewManager = control->view_manager();
  core::ViewManager::iterator viewItr;

  if (!args.front().as_string().empty())
    viewItr = viewManager->find(args.front().as_string());
  else
    viewItr = viewManager->find("default");

  if (viewItr == viewManager->end())
    throw torrent::input_error("Could not find view.");

  // Add some pre-parsing of the commands, so we don't spend time
  // parsing and searching command map for every single call.
  torrent::Object             resultRaw = torrent::Object::create_list();
  torrent::Object::list_type& result = resultRaw.as_list();

  for (core::View::const_iterator vItr = (*viewItr)->begin_visible(), vLast = (*viewItr)->end_visible(); vItr != vLast; vItr++) {
    torrent::Object::list_type& row = result.insert(result.end(), torrent::Object::create_list())->as_list();

    for (torrent::Object::list_const_iterator cItr = ++args.begin(), cLast = args.end(); cItr != args.end(); cItr++) {
      const std::string& cmd = cItr->as_string();
      row.push_back(rpc::parse_command(rpc::make_target(*vItr), cmd.c_str(), cmd.c_str() + cmd.size()).first);
    }
  }

  return resultRaw;
}

void
initialize_command_events() {
  ADD_VARIABLE_BOOL("check_hash", true);

  ADD_VARIABLE_BOOL("session_lock", true);
  ADD_VARIABLE_BOOL("session_on_completion", true);

  // Deprecated.
  ADD_COMMAND_LIST("on_insert",       rak::bind_ptr_fn(&apply_on_state_change, "event.download.inserted"));
  ADD_COMMAND_LIST("on_erase" ,       rak::bind_ptr_fn(&apply_on_state_change, "event.download.erased"));
  ADD_COMMAND_LIST("on_open",         rak::bind_ptr_fn(&apply_on_state_change, "event.download.opened"));
  ADD_COMMAND_LIST("on_close",        rak::bind_ptr_fn(&apply_on_state_change, "event.download.closed"));
  ADD_COMMAND_LIST("on_start",        rak::bind_ptr_fn(&apply_on_state_change, "event.download.resumed"));
  ADD_COMMAND_LIST("on_stop",         rak::bind_ptr_fn(&apply_on_state_change, "event.download.paused"));
  ADD_COMMAND_LIST("on_hash_queued",  rak::bind_ptr_fn(&apply_on_state_change, "event.download.hash_queued"));
  ADD_COMMAND_LIST("on_hash_removed", rak::bind_ptr_fn(&apply_on_state_change, "event.download.hash_removed"));
  ADD_COMMAND_LIST("on_finished",     rak::bind_ptr_fn(&apply_on_state_change, "event.download.finished"));

  ADD_COMMAND_STRING("on_ratio",      rak::ptr_fn(&apply_on_ratio));

  ADD_COMMAND_VOID("start_tied",      &apply_start_tied);
  ADD_COMMAND_VOID("stop_untied",     &apply_stop_untied);
  ADD_COMMAND_VOID("close_untied",    &apply_close_untied);
  ADD_COMMAND_VOID("remove_untied",   &apply_remove_untied);

  ADD_COMMAND_LIST("schedule",                rak::ptr_fn(&apply_schedule));
  ADD_COMMAND_STRING_UN("schedule_remove",    rak::make_mem_fun(control->command_scheduler(), &rpc::CommandScheduler::erase_str));

  ADD_COMMAND_STRING_UN("import",             std::ptr_fun(&apply_import));
  ADD_COMMAND_STRING_UN("try_import",         std::ptr_fun(&apply_try_import));

  ADD_COMMAND_LIST("load",                    rak::bind_ptr_fn(&apply_load, core::Manager::create_quiet | core::Manager::create_tied));
  ADD_COMMAND_LIST("load_verbose",            rak::bind_ptr_fn(&apply_load, core::Manager::create_tied));
  ADD_COMMAND_LIST("load_start",              rak::bind_ptr_fn(&apply_load, core::Manager::create_quiet | core::Manager::create_tied | core::Manager::create_start));
  ADD_COMMAND_LIST("load_start_verbose",      rak::bind_ptr_fn(&apply_load, core::Manager::create_tied  | core::Manager::create_start));
  ADD_COMMAND_LIST("load_raw",                rak::bind_ptr_fn(&apply_load, core::Manager::create_quiet | core::Manager::create_raw_data));
  ADD_COMMAND_LIST("load_raw_verbose",        rak::bind_ptr_fn(&apply_load, core::Manager::create_raw_data));
  ADD_COMMAND_LIST("load_raw_start",          rak::bind_ptr_fn(&apply_load, core::Manager::create_quiet | core::Manager::create_start | core::Manager::create_raw_data));

  ADD_COMMAND_VALUE_UN("close_low_diskspace", std::ptr_fun(&apply_close_low_diskspace));

  ADD_COMMAND_LIST("download_list",           rak::ptr_fn(&apply_download_list));
  ADD_COMMAND_LIST("d.multicall",             rak::ptr_fn(&d_multicall));
  ADD_COMMAND_COPY("call_download",           call_list, "i:", "");
}
