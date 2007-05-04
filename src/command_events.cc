// rTorrent - BitTorrent client
// Copyright (C) 2005-2006, Jari Sundell
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
#include <rak/file_stat.h>
#include <rak/path.h>
#include <sigc++/bind.h>
#include <torrent/rate.h>

#include "core/download.h"
#include "core/download_list.h"
#include "core/manager.h"
#include "utils/command_slot.h"
#include "utils/command_variable.h"
#include "utils/parse.h"

#include "globals.h"
#include "control.h"
#include "command_helpers.h"
#include "command_scheduler.h"

torrent::Object
apply_on_state_change(core::DownloadList::slot_map* slotMap, const torrent::Object& rawArgs) {
  const torrent::Object::list_type& args = rawArgs.as_list();

  if (args.size() < 2)
    throw torrent::input_error("Too few arguments.");

  if (args.front().as_string().empty())
    throw torrent::input_error("Empty key.");

  std::string key = "1_state_" + args.front().as_string();

  if (args.back().as_string().empty())
    slotMap->erase(key);
  else
    (*slotMap)[key] = sigc::bind(sigc::mem_fun(control->download_variables(), &utils::VariableMap::process_d_std_single),
                                 utils::convert_list_to_command(++args.begin(), args.end()));

  return torrent::Object();
}

torrent::Object
apply_stop_on_ratio(const torrent::Object& rawArgs) {
  const torrent::Object::list_type& args = rawArgs.as_list();

  if (args.empty())
    throw torrent::input_error("Too few arguments.");

  torrent::Object::list_type::const_iterator argItr = args.begin();

  // first argument:  minimum ratio to reach
  // second argument: minimum upload amount to reach [optional]
  // third argument:  maximum ratio to reach [optional]
  int64_t minRatio  = utils::convert_to_value(*argItr++);
  int64_t minUpload = argItr != args.end() ? utils::convert_to_value(*argItr++) : 0;
  int64_t maxRatio  = argItr != args.end() ? utils::convert_to_value(*argItr++) : 0;

  core::DownloadList* downloadList = control->core()->download_list();
  core::Manager::DListItr itr = downloadList->begin();

  while ((itr = std::find_if(itr, downloadList->end(), std::mem_fun(&core::Download::is_seeding)))
         != downloadList->end()) {
    int64_t totalDone   = (*itr)->download()->bytes_done();
    int64_t totalUpload = (*itr)->download()->up_rate()->total();

    if ((totalUpload >= minUpload && totalUpload * 100 >= totalDone * minRatio) ||
        (maxRatio > 0 && totalUpload * 100 > totalDone * maxRatio)) {
      downloadList->stop_try(*itr);
      (*itr)->set("ignore_commands", (int64_t)1);
    }

    ++itr;
  }

  return torrent::Object();
}

torrent::Object
apply_start_tied() {
  for (core::DownloadList::iterator itr = control->core()->download_list()->begin(); itr != control->core()->download_list()->end(); ++itr) {
    if ((*itr)->get_value("state") == 1)
      continue;

    rak::file_stat fs;
    const std::string& tiedToFile = (*itr)->get_string("tied_to_file");

    if (!tiedToFile.empty() && fs.update(rak::path_expand(tiedToFile)))
      control->core()->download_list()->start_try(*itr);
  }

  return torrent::Object();
}

torrent::Object
apply_stop_untied() {
  for (core::DownloadList::iterator itr = control->core()->download_list()->begin(); itr != control->core()->download_list()->end(); ++itr) {
    if ((*itr)->get_value("state") == 0)
      continue;

    rak::file_stat fs;
    const std::string& tiedToFile = (*itr)->get_string("tied_to_file");

    if (!tiedToFile.empty() && !fs.update(rak::path_expand(tiedToFile)))
      control->core()->download_list()->stop_try(*itr);
  }

  return torrent::Object();
}

torrent::Object
apply_close_untied() {
  for (core::DownloadList::iterator itr = control->core()->download_list()->begin(); itr != control->core()->download_list()->end(); ++itr) {
    rak::file_stat fs;
    const std::string& tiedToFile = (*itr)->get_string("tied_to_file");

    if (!tiedToFile.empty() && !fs.update(rak::path_expand(tiedToFile)) && control->core()->download_list()->stop_try(*itr))
      control->core()->download_list()->close(*itr);
  }

  return torrent::Object();
}

torrent::Object
apply_remove_untied() {
  for (core::DownloadList::iterator itr = control->core()->download_list()->begin(); itr != control->core()->download_list()->end(); ) {
    rak::file_stat fs;
    const std::string& tiedToFile = (*itr)->get_string("tied_to_file");

    if (!tiedToFile.empty() && !fs.update(rak::path_expand(tiedToFile)) && control->core()->download_list()->stop_try(*itr))
      itr = control->core()->download_list()->erase(itr);
    else
      ++itr;
  }

  return torrent::Object();
}

torrent::Object
apply_schedule(const torrent::Object& rawArgs) {
  const torrent::Object::list_type& args = rawArgs.as_list();

  if (args.size() < 4)
    throw torrent::input_error("Too few arguments.");

  torrent::Object::list_type::const_iterator itr = args.begin();

  const std::string& arg1 = (itr++)->as_string();
  const std::string& arg2 = (itr++)->as_string();
  const std::string& arg3 = (itr++)->as_string();

  control->command_scheduler()->parse(arg1, arg2, arg3, utils::convert_list_to_command(itr, args.end()));

  return torrent::Object();
}

void apply_load(const std::string& arg)               { control->core()->try_create_download_expand(arg, false, false, true); }
void apply_load_verbose(const std::string& arg)       { control->core()->try_create_download_expand(arg, false, true, true); }
void apply_load_start(const std::string& arg)         { control->core()->try_create_download_expand(arg, true, false, true); }
void apply_load_start_verbose(const std::string& arg) { control->core()->try_create_download_expand(arg, true, true, true); }

void apply_import(const std::string& path)     { if (!control->variable()->process_file(path.c_str())) throw torrent::input_error("Could not open option file: " + path); }
void apply_try_import(const std::string& path) { if (!control->variable()->process_file(path.c_str())) control->core()->push_log("Could not read resource file: " + path); }

void
apply_close_low_diskspace(int64_t arg) {
  core::Manager::DListItr itr = control->core()->download_list()->begin();

  while ((itr = std::find_if(itr, control->core()->download_list()->end(), std::mem_fun(&core::Download::is_downloading))) != control->core()->download_list()->end()) {
    if ((*itr)->file_list()->free_diskspace() < (uint64_t)arg) {
      control->core()->download_list()->close(*itr);

      (*itr)->set_hash_failed(true);
      (*itr)->set_message(std::string("Low diskspace."));
    }

    ++itr;
  }
}

void
initialize_command_events() {
  utils::VariableMap* variables = control->variable();
  core::DownloadList* downloadList = control->core()->download_list();

  ADD_VARIABLE_BOOL("check_hash", true);

  ADD_VARIABLE_BOOL("session_lock", true);
  ADD_VARIABLE_BOOL("session_on_completion", true);

  ADD_COMMAND_SLOT_PRIVATE("on_insert",       call_list, rak::bind_ptr_fn(&apply_on_state_change, &downloadList->slot_map_insert()));
  ADD_COMMAND_SLOT_PRIVATE("on_erase",        call_list, rak::bind_ptr_fn(&apply_on_state_change, &downloadList->slot_map_erase()));
  ADD_COMMAND_SLOT_PRIVATE("on_open",         call_list, rak::bind_ptr_fn(&apply_on_state_change, &downloadList->slot_map_open()));
  ADD_COMMAND_SLOT_PRIVATE("on_close",        call_list, rak::bind_ptr_fn(&apply_on_state_change, &downloadList->slot_map_close()));
  ADD_COMMAND_SLOT_PRIVATE("on_start",        call_list, rak::bind_ptr_fn(&apply_on_state_change, &downloadList->slot_map_start()));
  ADD_COMMAND_SLOT_PRIVATE("on_stop",         call_list, rak::bind_ptr_fn(&apply_on_state_change, &downloadList->slot_map_stop()));
  ADD_COMMAND_SLOT_PRIVATE("on_hash_queued",  call_list, rak::bind_ptr_fn(&apply_on_state_change, &downloadList->slot_map_hash_queued()));
  ADD_COMMAND_SLOT_PRIVATE("on_hash_removed", call_list, rak::bind_ptr_fn(&apply_on_state_change, &downloadList->slot_map_hash_removed()));
  ADD_COMMAND_SLOT_PRIVATE("on_hash_done",    call_list, rak::bind_ptr_fn(&apply_on_state_change, &downloadList->slot_map_hash_done()));
  ADD_COMMAND_SLOT_PRIVATE("on_finished",     call_list, rak::bind_ptr_fn(&apply_on_state_change, &downloadList->slot_map_finished()));

  ADD_COMMAND_SLOT_PRIVATE("stop_on_ratio",   call_list, rak::ptr_fn(&apply_stop_on_ratio));

  ADD_COMMAND_SLOT_PRIVATE("start_tied",      call_string, utils::object_fn(&apply_start_tied));
  ADD_COMMAND_SLOT_PRIVATE("stop_untied",     call_string, utils::object_fn(&apply_stop_untied));
  ADD_COMMAND_SLOT_PRIVATE("close_untied",    call_string, utils::object_fn(&apply_close_untied));
  ADD_COMMAND_SLOT_PRIVATE("remove_untied",   call_string, utils::object_fn(&apply_remove_untied));

  ADD_COMMAND_LIST("schedule",                rak::ptr_fn(&apply_schedule));
  ADD_COMMAND_STRING_UN("schedule_remove",    rak::make_mem_fun(control->command_scheduler(), &CommandScheduler::erase_str));

  ADD_COMMAND_STRING_UN("import",                std::ptr_fun(&apply_import));
  ADD_COMMAND_STRING_UN("try_import",            std::ptr_fun(&apply_try_import));

  ADD_COMMAND_STRING_UN("load",                  std::ptr_fun(&apply_load));
  ADD_COMMAND_STRING_UN("load_verbose",          std::ptr_fun(&apply_load_verbose));
  ADD_COMMAND_STRING_UN("load_start",            std::ptr_fun(&apply_load_start));
  ADD_COMMAND_STRING_UN("load_start_verbose",    std::ptr_fun(&apply_load_start_verbose));

  ADD_COMMAND_VALUE_UN("close_low_diskspace",    std::ptr_fun(&apply_close_low_diskspace));
}
