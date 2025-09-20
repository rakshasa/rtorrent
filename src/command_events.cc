#include "config.h"

#include <functional>
#include <cstdio>
#include <rak/error_number.h>
#include <rak/file_stat.h>
#include <rak/path.h>
#include <rak/string_manip.h>
#include <torrent/rate.h>
#include <torrent/hash_string.h>
#include <torrent/utils/log.h>
#include <torrent/utils/directory_events.h>

#include "globals.h"
#include "control.h"
#include "command_helpers.h"
#include "core/download.h"
#include "core/download_list.h"
#include "core/manager.h"
#include "core/view_manager.h"
#include "rpc/command_scheduler.h"
#include "rpc/parse.h"
#include "rpc/parse_commands.h"

torrent::Object
apply_on_ratio(const torrent::Object& rawArgs) {
  auto& group_name = rawArgs.as_string();
  auto  view_itr = control->view_manager()->find(rpc::commands.call("group." + group_name + ".view", rpc::make_target()).as_string());

  if (view_itr == control->view_manager()->end())
    throw torrent::input_error("Could not find view.");

  // first argument:  minimum ratio to reach
  // second argument: minimum upload amount to reach [optional]
  // third argument:  maximum ratio to reach [optional]
  int64_t min_ratio  = rpc::commands.call("group." + group_name + ".ratio.min", rpc::make_target()).as_value();
  int64_t max_ratio  = rpc::commands.call("group." + group_name + ".ratio.max", rpc::make_target()).as_value();
  int64_t min_upload = rpc::commands.call("group." + group_name + ".ratio.upload", rpc::make_target()).as_value();

  std::vector<core::Download*> downloads;

  for  (auto itr = (*view_itr)->begin_visible(), last = (*view_itr)->end_visible(); itr != last; itr++) {
    if (!(*itr)->is_seeding() || rpc::call_command_value("d.ignore_commands", rpc::make_target(*itr)) != 0)
      continue;

    int64_t total_done   = (*itr)->download()->bytes_done();
    int64_t total_upload = (*itr)->info()->up_rate()->total();

    if (!(total_upload >= min_upload && total_upload * 100 >= total_done * min_ratio) &&
        !(max_ratio > 0 && total_upload * 100 > total_done * max_ratio))
      continue;

    downloads.push_back(*itr);
  }

  auto ratio_command = "group." + group_name + ".ratio.command";

  for (const auto& download : downloads)
    rpc::commands.call_catch(ratio_command, rpc::make_target(download), torrent::Object(), "Ratio reached, but command failed: ");

  return torrent::Object();
}

torrent::Object
apply_start_tied() {
  for (const auto& download : *control->core()->download_list()) {
    if (rpc::call_command_value("d.state", rpc::make_target(download)) == 1)
      continue;

    rak::file_stat fs;
    const std::string& tied_to_file = rpc::call_command_string("d.tied_to_file", rpc::make_target(download));

    if (!tied_to_file.empty() && fs.update(rak::path_expand(tied_to_file)))
      rpc::parse_command_single(rpc::make_target(download), "d.try_start=");
  }

  return torrent::Object();
}

torrent::Object
apply_stop_untied() {
  for (const auto& download : *control->core()->download_list()) {
    if (rpc::call_command_value("d.state", rpc::make_target(download)) == 0)
      continue;

    rak::file_stat fs;
    const std::string& tied_to_file = rpc::call_command_string("d.tied_to_file", rpc::make_target(download));

    if (!tied_to_file.empty() && !fs.update(rak::path_expand(tied_to_file)))
      rpc::parse_command_single(rpc::make_target(download), "d.try_stop=");
  }

  return torrent::Object();
}

torrent::Object
apply_close_untied() {
  for (const auto& download : *control->core()->download_list()) {
    rak::file_stat fs;
    const std::string& tied_to_file = rpc::call_command_string("d.tied_to_file", rpc::make_target(download));

    if (rpc::call_command_value("d.ignore_commands", rpc::make_target(download)) == 0 && !tied_to_file.empty() && !fs.update(rak::path_expand(tied_to_file)))
      rpc::parse_command_single(rpc::make_target(download), "d.try_close=");
  }

  return torrent::Object();
}

torrent::Object
apply_remove_untied() {
  for (auto itr = control->core()->download_list()->begin(); itr != control->core()->download_list()->end(); ) {
    rak::file_stat fs;
    const std::string& tied_to_file = rpc::call_command_string("d.tied_to_file", rpc::make_target(*itr));

    if (!tied_to_file.empty() && !fs.update(rak::path_expand(tied_to_file))) {
      // Need to clear tied_to_file so it doesn't try to delete it.
      rpc::call_command("d.tied_to_file.set", std::string(), rpc::make_target(*itr));

      itr = control->core()->download_list()->erase(itr);

    } else {
      ++itr;
    }
  }

  return torrent::Object();
}

torrent::Object
apply_schedule(const torrent::Object::list_type& args) {
  if (args.size() != 4)
    throw torrent::input_error("Wrong number of arguments.");

  torrent::Object::list_const_iterator itr = args.begin();

  auto& arg1 = (itr++)->as_string();
  auto& arg2 = (itr++)->as_string();
  auto& arg3 = (itr++)->as_string();

  control->command_scheduler()->parse(arg1, arg2, arg3, *itr);

  return torrent::Object();
}

torrent::Object
apply_load(const torrent::Object::list_type& args, int flags) {
  torrent::Object::list_const_iterator argsItr = args.begin();

  if (argsItr == args.end())
    throw torrent::input_error("Too few arguments.");

  auto& filename = argsItr->as_string();
  core::Manager::command_list_type commands;

  while (++argsItr != args.end())
    commands.push_back(argsItr->as_string());

  control->core()->try_create_download_expand(filename, flags, commands);

  return torrent::Object();
}

void apply_import(const std::string& path)     { if (!rpc::parse_command_file(path)) throw torrent::input_error("Could not open option file: " + path); }
void apply_try_import(const std::string& path) { if (!rpc::parse_command_file(path)) control->core()->push_log_std("Could not read resource file: " + path); }

torrent::Object
apply_close_low_diskspace(int64_t arg, uint32_t skip_priority) {
  bool closed = false;

  for (auto download : *control->core()->download_list()) {
    if (!download->is_downloading())
      continue;
    if (download->priority() >= skip_priority)
      continue;
    if (download->file_list()->free_diskspace() >= (uint64_t)arg)
      continue;

    control->core()->download_list()->close(download);

    download->set_hash_failed(true);
    download->set_message(std::string("Low diskspace."));

    closed = true;
  }

  if (closed)
    lt_log_print(torrent::LOG_TORRENT_ERROR, "Closed torrents due to low diskspace.");

  return torrent::Object();
}

torrent::Object
apply_download_list(const torrent::Object::list_type& args) {
  torrent::Object::list_const_iterator argsItr = args.begin();

  core::ViewManager* viewManager = control->view_manager();
  core::ViewManager::iterator view_itr;

  if (argsItr != args.end() && !argsItr->as_string().empty())
    view_itr = viewManager->find((argsItr++)->as_string());
  else
    view_itr = viewManager->find("default");

  if (view_itr == viewManager->end())
    throw torrent::input_error("Could not find view.");

  torrent::Object result = torrent::Object::create_list();
  torrent::Object::list_type& resultList = result.as_list();

  for (core::View::const_iterator itr = (*view_itr)->begin_visible(), last = (*view_itr)->end_visible(); itr != last; itr++) {
    const torrent::HashString* hashString = &(*itr)->info()->hash();

    resultList.push_back(rak::transform_hex(hashString->begin(), hashString->end()));
  }

  return result;
}

torrent::Object
d_multicall(const torrent::Object::list_type& args) {
  if (args.empty())
    throw torrent::input_error("Too few arguments.");

  core::ViewManager* viewManager = control->view_manager();
  core::ViewManager::iterator view_itr;

  if (!args.front().as_string().empty())
    view_itr = viewManager->find(args.front().as_string());
  else
    view_itr = viewManager->find("default");

  if (view_itr == viewManager->end())
    throw torrent::input_error("Could not find view.");

  // Add some pre-parsing of the commands, so we don't spend time
  // parsing and searching command map for every single call.
  std::vector<core::Download*> dlist((*view_itr)->begin_visible(), (*view_itr)->end_visible());

  torrent::Object             resultRaw = torrent::Object::create_list();
  torrent::Object::list_type& result = resultRaw.as_list();

  for (auto download : dlist) {
    torrent::Object::list_type& row = result.insert(result.end(), torrent::Object::create_list())->as_list();

    for (torrent::Object::list_const_iterator cItr = ++args.begin(); cItr != args.end(); cItr++) {
      auto& cmd = cItr->as_string();
      row.push_back(rpc::parse_command(rpc::make_target(download), cmd.c_str(), cmd.c_str() + cmd.size()).first);
    }
  }

  return resultRaw;
}

torrent::Object
d_multicall_filtered(const torrent::Object::list_type& args) {
  if (args.size() < 2)
    throw torrent::input_error("d.multicall.filtered requires at least 2 arguments.");
  torrent::Object::list_const_iterator arg = args.begin();

  // Find the given view
  core::ViewManager* viewManager = control->view_manager();
  core::ViewManager::iterator view_itr = viewManager->find(arg->as_string().empty() ? "default" : arg->as_string());

  if (view_itr == viewManager->end())
    throw torrent::input_error("Could not find view '" + arg->as_string() + "'.");

  // Make a filtered copy of the current item list
  core::View::base_type dlist;
  (*view_itr)->filter_by(*++arg, dlist);

  // Generate result by iterating over all items
  torrent::Object             resultRaw = torrent::Object::create_list();
  torrent::Object::list_type& result = resultRaw.as_list();
  ++arg;  // skip to first command

  for (const auto& item : dlist) {
    // Add empty row to result
    torrent::Object::list_type& row = result.insert(result.end(), torrent::Object::create_list())->as_list();

    // Call the provided commands and assemble their results
    for (torrent::Object::list_const_iterator command = arg; command != args.end(); command++) {
      auto& cmdstr = command->as_string();
      row.push_back(rpc::parse_command(rpc::make_target(item), cmdstr.c_str(), cmdstr.c_str() + cmdstr.size()).first);
    }
  }

  return resultRaw;
}

static void
call_watch_command(const std::string& command, const std::string& path) {
  rpc::commands.call_catch(command.c_str(), rpc::make_target(), path);
}

torrent::Object
directory_watch_added(const torrent::Object::list_type& args) {
  if (args.size() != 2)
    throw torrent::input_error("Too few arguments.");

  auto& path = args.front().as_string();
  auto& command = args.back().as_string();

  if (!control->directory_events()->open())
    throw torrent::input_error("Could not open inotify:" + std::string(rak::error_number::current().c_str()));

  control->directory_events()->notify_on(path.c_str(),
                                         torrent::directory_events::flag_on_added | torrent::directory_events::flag_on_updated,
                                         std::bind(&call_watch_command, command, std::placeholders::_1));
  return torrent::Object();
}

void
initialize_command_events() {
  CMD2_ANY_STRING  ("on_ratio",        std::bind(&apply_on_ratio, std::placeholders::_2));

  CMD2_ANY         ("start_tied",      std::bind(&apply_start_tied));
  CMD2_ANY         ("stop_untied",     std::bind(&apply_stop_untied));
  CMD2_ANY         ("close_untied",    std::bind(&apply_close_untied));
  CMD2_ANY         ("remove_untied",   std::bind(&apply_remove_untied));

  // TODO: Deprecate schedule2 in the future.
  CMD2_ANY_LIST    ("schedule",         std::bind(&apply_schedule, std::placeholders::_2));
  CMD2_ANY_LIST    ("schedule2",        std::bind(&apply_schedule, std::placeholders::_2));
  CMD2_ANY_STRING_V("schedule.remove",  std::bind(&rpc::CommandScheduler::erase_str, control->command_scheduler(), std::placeholders::_2));
  CMD2_ANY_STRING_V("schedule_remove2", std::bind(&rpc::CommandScheduler::erase_str, control->command_scheduler(), std::placeholders::_2));

  CMD2_ANY_STRING_V("import",          std::bind(&apply_import, std::placeholders::_2));
  CMD2_ANY_STRING_V("try_import",      std::bind(&apply_try_import, std::placeholders::_2));

  CMD2_ANY_LIST    ("load.normal",        std::bind(&apply_load, std::placeholders::_2, core::Manager::create_quiet | core::Manager::create_tied));
  CMD2_ANY_LIST    ("load.verbose",       std::bind(&apply_load, std::placeholders::_2, core::Manager::create_tied));
  CMD2_ANY_LIST    ("load.start",         std::bind(&apply_load, std::placeholders::_2,
                                                         core::Manager::create_quiet | core::Manager::create_tied | core::Manager::create_start));
  CMD2_ANY_LIST    ("load.start_verbose", std::bind(&apply_load, std::placeholders::_2, core::Manager::create_tied  | core::Manager::create_start));
  CMD2_ANY_LIST    ("load.raw",           std::bind(&apply_load, std::placeholders::_2, core::Manager::create_quiet | core::Manager::create_raw_data));
  CMD2_ANY_LIST    ("load.raw_verbose",   std::bind(&apply_load, std::placeholders::_2, core::Manager::create_raw_data));
  CMD2_ANY_LIST    ("load.raw_start",     std::bind(&apply_load, std::placeholders::_2,
                                                         core::Manager::create_quiet | core::Manager::create_start | core::Manager::create_raw_data));
  CMD2_ANY_LIST    ("load.raw_start_verbose", std::bind(&apply_load, std::placeholders::_2, core::Manager::create_start | core::Manager::create_raw_data));

  CMD2_ANY_VALUE   ("close_low_diskspace",        std::bind(&apply_close_low_diskspace, std::placeholders::_2, 99));
  CMD2_ANY_VALUE   ("close_low_diskspace.normal", std::bind(&apply_close_low_diskspace, std::placeholders::_2, 3));

  CMD2_ANY_LIST    ("download_list",       std::bind(&apply_download_list, std::placeholders::_2));
  CMD2_ANY_LIST    ("d.multicall2",        std::bind(&d_multicall, std::placeholders::_2));
  CMD2_ANY_LIST    ("d.multicall.filtered", std::bind(&d_multicall_filtered, std::placeholders::_2));

  CMD2_ANY_LIST    ("directory.watch.added", std::bind(&directory_watch_added, std::placeholders::_2));
}
