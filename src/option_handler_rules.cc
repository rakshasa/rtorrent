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

#include <cstdio>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <rak/file_stat.h>
#include <rak/fs_stat.h>
#include <rak/functional.h>
#include <rak/path.h>
#include <rak/string_manip.h>
#include <torrent/object.h>
#include <torrent/chunk_manager.h>
#include <torrent/connection_manager.h>
#include <torrent/exceptions.h>
#include <torrent/file.h>
#include <torrent/path.h>
#include <torrent/rate.h>
#include <torrent/torrent.h>
#include <torrent/tracker.h>
#include <torrent/tracker_list.h>

#include "core/download.h"
#include "core/download_list.h"
#include "core/download_store.h"
#include "core/manager.h"
#include "core/scheduler.h"
#include "core/view_manager.h"
#include "ui/root.h"
#include "utils/directory.h"
#include "utils/variable_generic.h"
#include "utils/variable_map.h"

#include "globals.h"
#include "control.h"
#include "option_handler_rules.h"
#include "command_scheduler.h"

namespace core {
  extern void
  path_expand(std::vector<std::string>* paths, const std::string& pattern);
}

void
apply_hash_read_ahead(__UNUSED Control* m, int arg) {
  torrent::set_hash_read_ahead(arg << 20);
}

void
apply_hash_interval(__UNUSED Control* m, int arg) {
  torrent::set_hash_interval(arg * 1000);
}

// The arg string *must* have been checked with validate_port_range
// first.
void
apply_port_range(Control* m, const std::string& arg) {
  int a = 0, b = 0;
    
  std::sscanf(arg.c_str(), "%i-%i", &a, &b);

  m->core()->set_port_range(a, b);
}

void
apply_load(Control* m, const std::string& arg) {
  m->core()->try_create_download_expand(arg, false, false, true);
}

void
apply_load_verbose(Control* m, const std::string& arg) {
  m->core()->try_create_download_expand(arg, false, true, true);
}

void
apply_load_start(Control* m, const std::string& arg) {
  m->core()->try_create_download_expand(arg, true, false, true);
}

void
apply_load_start_verbose(Control* m, const std::string& arg) {
  m->core()->try_create_download_expand(arg, true, true, true);
}

void
apply_start_tied(Control* m) {
  for (core::DownloadList::iterator itr = m->core()->download_list()->begin(); itr != m->core()->download_list()->end(); ++itr) {
    if ((*itr)->variable_value("state") == 1)
      continue;

    rak::file_stat fs;
    const std::string& tiedToFile = (*itr)->variable_string("tied_to_file");

    if (!tiedToFile.empty() && fs.update(rak::path_expand(tiedToFile)))
      m->core()->download_list()->start_try(*itr);
  }
}

void
apply_stop_untied(Control* m) {
  for (core::DownloadList::iterator itr = m->core()->download_list()->begin(); itr != m->core()->download_list()->end(); ++itr) {
    if ((*itr)->variable_value("state") == 0)
      continue;

    rak::file_stat fs;
    const std::string& tiedToFile = (*itr)->variable_string("tied_to_file");

    if (!tiedToFile.empty() && !fs.update(rak::path_expand(tiedToFile)))
      m->core()->download_list()->stop_try(*itr);
  }
}

void
apply_close_untied(Control* m) {
  for (core::DownloadList::iterator itr = m->core()->download_list()->begin(); itr != m->core()->download_list()->end(); ++itr) {
    rak::file_stat fs;
    const std::string& tiedToFile = (*itr)->variable_string("tied_to_file");

    if (!tiedToFile.empty() && !fs.update(rak::path_expand(tiedToFile)) && m->core()->download_list()->stop_try(*itr))
      m->core()->download_list()->close(*itr);
  }
}

void
apply_remove_untied(Control* m) {
  for (core::DownloadList::iterator itr = m->core()->download_list()->begin(); itr != m->core()->download_list()->end(); ) {
    rak::file_stat fs;
    const std::string& tiedToFile = (*itr)->variable_string("tied_to_file");

    if (!tiedToFile.empty() && !fs.update(rak::path_expand(tiedToFile)) && m->core()->download_list()->stop_try(*itr))
      itr = m->core()->download_list()->erase(itr);
    else
      ++itr;
  }
}

void
apply_close_low_diskspace(Control* m, int64_t arg) {
  core::Manager::DListItr itr = m->core()->download_list()->begin();

  while ((itr = std::find_if(itr, m->core()->download_list()->end(), std::mem_fun(&core::Download::is_downloading))) != m->core()->download_list()->end()) {
    rak::fs_stat stat;
    std::string path = (*itr)->file_list()->root_dir() + (*itr)->file_list()->get(0).path()->as_string();

    if (!stat.update(path)) {
      m->core()->push_log(std::string("Cannot read free diskspace: ") + strerror(errno) + " for " + path);

    } else if (stat.bytes_avail() < arg) {
      m->core()->download_list()->close(*itr);

      (*itr)->set_hash_failed(true);
      (*itr)->set_message(std::string("Low diskspace"));
    }

    ++itr;
  }
}

void
apply_stop_on_ratio(Control* m, const std::string& arg) {
  int64_t minRatio = 0;  // first argument:  minimum ratio to reach
  int64_t minUpload = 0; // second argument: minimum upload amount to reach [optional]
  int64_t maxRatio = 0;  // third argument:  maximum ratio to reach [optional]

  rak::split_iterator_t<std::string> sitr = rak::split_iterator(arg, ',');

  utils::Variable::string_to_value_unit(rak::trim(*sitr).c_str(), &minRatio, 0, 1);

  if (++sitr != rak::split_iterator(arg))
    utils::Variable::string_to_value_unit(rak::trim(*sitr).c_str(), &minUpload, 0, 1);

  if (++sitr != rak::split_iterator(arg))
    utils::Variable::string_to_value_unit(rak::trim(*sitr).c_str(), &maxRatio, 0, 1);

  core::Manager::DListItr itr = m->core()->download_list()->begin();

  while ((itr = std::find_if(itr, m->core()->download_list()->end(), std::mem_fun(&core::Download::is_seeding))) != m->core()->download_list()->end()) {
    int64_t totalUpload = (*itr)->download()->up_rate()->total();
    int64_t totalDone = (*itr)->download()->bytes_done();

    if ((totalUpload >= minUpload && totalUpload * 100 >= totalDone * minRatio) || (maxRatio > 0 && totalUpload * 100 > totalDone * maxRatio)) {
      m->core()->download_list()->stop_try(*itr);
      (*itr)->variable()->set("ignore_commands", (int64_t)1);
    }

    ++itr;
  }
}

void
apply_encoding_list(__UNUSED Control* m, const std::string& arg) {
  torrent::encoding_list()->push_back(arg);
}

void
apply_encryption(const std::string& arg) {
  rak::split_iterator_t<std::string> sitr = rak::split_iterator(arg, ',');
  uint32_t options_mask = torrent::ConnectionManager::encryption_none;

  while (sitr != rak::split_iterator(arg)) {
    std::string opt = rak::trim(*sitr);
    ++sitr;

    if (opt == "none")
      options_mask = torrent::ConnectionManager::encryption_none;
    else if (opt == "allow_incoming")
      options_mask |= torrent::ConnectionManager::encryption_allow_incoming;
    else if (opt == "try_outgoing")
      options_mask |= torrent::ConnectionManager::encryption_try_outgoing;
    else if (opt == "require")
      options_mask |= torrent::ConnectionManager::encryption_require;
    else if (opt == "require_RC4" || opt == "require_rc4")
      options_mask |= torrent::ConnectionManager::encryption_require_RC4;
    else if (opt == "enable_retry")
      options_mask |= torrent::ConnectionManager::encryption_enable_retry;
    else if (opt == "prefer_plaintext")
      options_mask |= torrent::ConnectionManager::encryption_prefer_plaintext;
    else
      throw torrent::input_error("Invalid encryption option '" + opt + "'.");
  }

  torrent::connection_manager()->set_encryption_options(options_mask);
}

void
apply_enable_trackers(Control* m, __UNUSED const std::string& arg) {
  bool state = (arg != "no");

  for (core::Manager::DListItr itr = m->core()->download_list()->begin(), last = m->core()->download_list()->end(); itr != last; ++itr) {

    torrent::TrackerList tl = (*itr)->download()->tracker_list();

    for (int i = 0, last = tl.size(); i < last; ++i)
      if (state)
        tl.get(i).enable();
      else
        tl.get(i).disable();

    if (state && !control->variable()->get_value("use_udp_trackers"))
      (*itr)->enable_udp_trackers(false);
  }    
}

void
apply_tos(const std::string& arg) {
  utils::Variable::value_type value;
  torrent::ConnectionManager* cm = torrent::connection_manager();

  if (arg == "default")
    value = torrent::ConnectionManager::iptos_default;

  else if (arg == "lowdelay")
    value = torrent::ConnectionManager::iptos_lowdelay;

  else if (arg == "throughput")
    value = torrent::ConnectionManager::iptos_throughput;

  else if (arg == "reliability")
    value = torrent::ConnectionManager::iptos_reliability;

  else if (arg == "mincost")
    value = torrent::ConnectionManager::iptos_mincost;

  else if (!utils::Variable::string_to_value_unit_nothrow(arg.c_str(), &value, 16, 0))
    throw torrent::input_error("Invalid TOS identifier.");

  cm->set_priority(value);
}

void
apply_view_filter(Control* control, const std::string& arg) {
  rak::split_iterator_t<std::string> itr = rak::split_iterator(arg, ',');

  std::string name = rak::trim(*itr);
  
  if (name.empty())
    throw torrent::input_error("First argument must be a string.");

  core::ViewManager::filter_args filterArgs;

  while (++itr != rak::split_iterator(arg)) {
    filterArgs.push_back(rak::trim(*itr));

    if (filterArgs.back().empty())
      throw torrent::input_error("One of the arguments is empty.");
  }

  control->view_manager()->set_filter(name, filterArgs);
}

void
apply_view_filter_on(Control* control, const std::string& arg) {
  rak::split_iterator_t<std::string> itr = rak::split_iterator(arg, ',');

  std::string name = rak::trim(*itr);
  
  if (name.empty())
    throw torrent::input_error("First argument must be a string.");

  core::ViewManager::filter_args filterArgs;

  while (++itr != rak::split_iterator(arg)) {
    filterArgs.push_back(rak::trim(*itr));

    if (filterArgs.back().empty())
      throw torrent::input_error("One of the arguments is empty.");
  }

  control->view_manager()->set_filter_on(name, filterArgs);
}

void
apply_view_sort(Control* control, const std::string& arg) {
  rak::split_iterator_t<std::string> itr = rak::split_iterator(arg, ',');

  std::string name = rak::trim(*itr);
  ++itr;

  if (name.empty())
    throw torrent::input_error("First argument must be a string.");

  // Need some generic tools for this, rather than hacking up
  // something every time...
  std::string arg1;
  int32_t value = 0;

  if (itr != rak::split_iterator(arg) && !(arg1 = *itr).empty()) {
    char* endPtr;

    if ((value = strtol(arg1.c_str(), &endPtr, 0)) < 0 || *endPtr != '\0')
      throw torrent::input_error("Second argument must be a value.");
  }
      
  control->view_manager()->sort(name, value);
}

void
apply_view_sort_current(Control* control, const std::string& arg) {
  rak::split_iterator_t<std::string> itr = rak::split_iterator(arg, ',');

  std::string name = rak::trim(*itr);
  
  if (name.empty())
    throw torrent::input_error("First argument must be a string.");

  core::ViewManager::sort_args sortArgs;

  while (++itr != rak::split_iterator(arg)) {
    sortArgs.push_back(rak::trim(*itr));

    if (sortArgs.back().empty())
      throw torrent::input_error("One of the arguments is empty.");
  }

  control->view_manager()->set_sort_current(name, sortArgs);
}

void
apply_view_sort_new(Control* control, const std::string& arg) {
  rak::split_iterator_t<std::string> itr = rak::split_iterator(arg, ',');

  std::string name = rak::trim(*itr);
  
  if (name.empty())
    throw torrent::input_error("First argument must be a string.");

  core::ViewManager::sort_args sortArgs;

  while (++itr != rak::split_iterator(arg)) {
    sortArgs.push_back(rak::trim(*itr));

    if (sortArgs.back().empty())
      throw torrent::input_error("One of the arguments is empty.");
  }

  control->view_manager()->set_sort_new(name, sortArgs);
}

void
apply_import(const std::string& path) {
  if (!control->variable()->process_file(path))
    throw torrent::input_error("Could not open option file: " + path);
}

void
apply_try_import(const std::string& path) {
  if (!control->variable()->process_file(path))
    control->core()->push_log("Could not read resource file: " + path);
}

void
initialize_option_handler(Control* c) {
  utils::VariableMap* variables = control->variable();

  variables->insert("check_hash",            new utils::VariableBool(true));
  variables->insert("use_udp_trackers",      new utils::VariableBool(true));
  variables->insert("port_open",             new utils::VariableBool(true));
  variables->insert("port_random",           new utils::VariableBool(true));

  variables->insert("tracker_dump",          new utils::VariableAny(std::string()));

  variables->insert("session",               new utils::VariableStringSlot(rak::mem_fn(control->core()->download_store(), &core::DownloadStore::path),
                                                                           rak::mem_fn(control->core()->download_store(), &core::DownloadStore::set_path)));
  variables->insert("session_lock",          new utils::VariableBool(true));
  variables->insert("session_on_completion", new utils::VariableBool(true));
  variables->insert("session_save",          new utils::VariableVoidSlot(rak::mem_fn(c->core()->download_list(), &core::DownloadList::session_save)));

  variables->insert("connection_leech",      new utils::VariableAny("leech"));
  variables->insert("connection_seed",       new utils::VariableAny("seed"));

  variables->insert("directory",             new utils::VariableAny("./"));

  variables->insert("tos",                   new utils::VariableStringSlot(rak::value_fn(std::string()), rak::ptr_fn(&apply_tos)));

  variables->insert("bind",                  new utils::VariableStringSlot(rak::mem_fn(control->core(), &core::Manager::bind_address),
                                                                           rak::mem_fn(control->core(), &core::Manager::set_bind_address)));
  variables->insert("ip",                    new utils::VariableStringSlot(rak::mem_fn(control->core(), &core::Manager::local_address),
                                                                           rak::mem_fn(control->core(), &core::Manager::set_local_address)));
  variables->insert("connection_proxy",      new utils::VariableStringSlot(rak::mem_fn(control->core(), &core::Manager::proxy_address),
                                                                           rak::mem_fn(control->core(), &core::Manager::set_proxy_address)));

  variables->insert("http_proxy",            new utils::VariableStringSlot(rak::mem_fn(c->core()->get_poll_manager()->get_http_stack(), &core::CurlStack::http_proxy),
                                                                           rak::mem_fn(c->core()->get_poll_manager()->get_http_stack(), &core::CurlStack::set_http_proxy)));

  variables->insert("min_peers",             new utils::VariableValue(40));
  variables->insert("max_peers",             new utils::VariableValue(100));
  variables->insert("min_peers_seed",        new utils::VariableValue(-1));
  variables->insert("max_peers_seed",        new utils::VariableValue(-1));
  variables->insert("max_uploads",           new utils::VariableValue(15));
  variables->insert("max_chunks_queued",     new utils::VariableValue(0));

  variables->insert("download_rate",         new utils::VariableValueSlot(rak::ptr_fn(&torrent::down_throttle), rak::mem_fn(control->ui(), &ui::Root::set_down_throttle_i64),
                                                                          0, (1 << 10)));
  variables->insert("upload_rate",           new utils::VariableValueSlot(rak::ptr_fn(&torrent::up_throttle), rak::mem_fn(control->ui(), &ui::Root::set_up_throttle_i64),
                                                                          0, (1 << 10)));

  variables->insert("tracker_numwant",       new utils::VariableValue(-1));

  variables->insert("hash_max_tries",        new utils::VariableValueSlot(rak::ptr_fn(&torrent::hash_max_tries), rak::ptr_fn(&torrent::set_hash_max_tries)));
  variables->insert("max_open_files",        new utils::VariableValueSlot(rak::ptr_fn(&torrent::max_open_files), rak::ptr_fn(&torrent::set_max_open_files)));
  variables->insert("max_open_sockets",      new utils::VariableValueSlot(rak::ptr_fn(&torrent::max_open_sockets), rak::ptr_fn(&torrent::set_max_open_sockets)));

  variables->insert("print",                 new utils::VariableStringSlot(rak::value_fn(std::string()), rak::mem_fn(control->core(), &core::Manager::push_log)));
  variables->insert("import",                new utils::VariableStringSlot(rak::value_fn(std::string()), rak::ptr_fn(&apply_import)));
  variables->insert("try_import",            new utils::VariableStringSlot(rak::value_fn(std::string()), rak::ptr_fn(&apply_try_import)));

  variables->insert("view_add",              new utils::VariableStringSlot(rak::value_fn(std::string()), rak::mem_fn(c->view_manager(), &core::ViewManager::insert_throw)));
  variables->insert("view_filter",           new utils::VariableStringSlot(rak::value_fn(std::string()), rak::bind_ptr_fn(&apply_view_filter, c)));
  variables->insert("view_filter_on",        new utils::VariableStringSlot(rak::value_fn(std::string()), rak::bind_ptr_fn(&apply_view_filter_on, c)));

  variables->insert("view_sort",             new utils::VariableStringSlot(rak::value_fn(std::string()), rak::bind_ptr_fn(&apply_view_sort, c)));
  variables->insert("view_sort_new",         new utils::VariableStringSlot(rak::value_fn(std::string()), rak::bind_ptr_fn(&apply_view_sort_new, c)));
  variables->insert("view_sort_current",     new utils::VariableStringSlot(rak::value_fn(std::string()), rak::bind_ptr_fn(&apply_view_sort_current, c)));

  variables->insert("key_layout",            new utils::VariableAny(std::string("qwerty")));

  variables->insert("schedule",              new utils::VariableStringSlot(rak::value_fn(std::string()), rak::mem_fn(c->command_scheduler(), &CommandScheduler::parse)));
  variables->insert("schedule_remove",       new utils::VariableStringSlot(rak::value_fn(std::string()),
                                                                           rak::mem_fn<const std::string&>(c->command_scheduler(), &CommandScheduler::erase)));

  variables->insert("download_scheduler",    new utils::VariableVoidSlot(rak::mem_fn(control->scheduler(), &core::Scheduler::update)));

  variables->insert("send_buffer_size",      new utils::VariableValueSlot(rak::mem_fn(torrent::connection_manager(), &torrent::ConnectionManager::send_buffer_size),
                                                                          rak::mem_fn(torrent::connection_manager(), &torrent::ConnectionManager::set_send_buffer_size)));
  
  variables->insert("receive_buffer_size",   new utils::VariableValueSlot(rak::mem_fn(torrent::connection_manager(), &torrent::ConnectionManager::receive_buffer_size),
                                                                          rak::mem_fn(torrent::connection_manager(), &torrent::ConnectionManager::set_receive_buffer_size)));
  
  variables->insert("max_memory_usage",      new utils::VariableValueSlot(rak::mem_fn(torrent::chunk_manager(), &torrent::ChunkManager::max_memory_usage),
                                                                          rak::mem_fn(torrent::chunk_manager(), &torrent::ChunkManager::set_max_memory_usage)));

  variables->insert("safe_sync",             new utils::VariableValueSlot(rak::mem_fn(torrent::chunk_manager(), &torrent::ChunkManager::safe_sync),
                                                                          rak::mem_fn(torrent::chunk_manager(), &torrent::ChunkManager::set_safe_sync)));

  variables->insert("timeout_sync",          new utils::VariableValueSlot(rak::mem_fn(torrent::chunk_manager(), &torrent::ChunkManager::timeout_sync),
                                                                          rak::mem_fn(torrent::chunk_manager(), &torrent::ChunkManager::set_timeout_sync)));

  variables->insert("timeout_safe_sync",     new utils::VariableValueSlot(rak::mem_fn(torrent::chunk_manager(), &torrent::ChunkManager::timeout_safe_sync),
                                                                          rak::mem_fn(torrent::chunk_manager(), &torrent::ChunkManager::set_timeout_safe_sync)));

  variables->insert("preload_min_pipelined", new utils::VariableValueSlot(rak::mem_fn(torrent::chunk_manager(), &torrent::ChunkManager::preload_min_pipelined),
                                                                          rak::mem_fn(torrent::chunk_manager(), &torrent::ChunkManager::set_preload_min_pipelined)));

  variables->insert("preload_min_size",      new utils::VariableValueSlot(rak::mem_fn(torrent::chunk_manager(), &torrent::ChunkManager::preload_min_size),
                                                                          rak::mem_fn(torrent::chunk_manager(), &torrent::ChunkManager::set_preload_min_size)));

  variables->insert("preload_required_rate", new utils::VariableValueSlot(rak::mem_fn(torrent::chunk_manager(), &torrent::ChunkManager::preload_required_rate),
                                                                          rak::mem_fn(torrent::chunk_manager(), &torrent::ChunkManager::set_preload_required_rate)));

  variables->insert("port_range",            new utils::VariableStringSlot(rak::value_fn(std::string()), rak::bind_ptr_fn(&apply_port_range, c)));

  variables->insert("hash_read_ahead",       new utils::VariableValueSlot(rak::ptr_fn(torrent::hash_read_ahead), rak::bind_ptr_fn(&apply_hash_read_ahead, c)));
  variables->insert("hash_interval",         new utils::VariableValueSlot(rak::ptr_fn(torrent::hash_interval), rak::bind_ptr_fn(&apply_hash_interval, c)));

  variables->insert("umask",                 new utils::VariableValueSlot(rak::mem_fn(control, &Control::umask), rak::mem_fn(control, &Control::set_umask), 8));
  variables->insert("working_directory",     new utils::VariableStringSlot(rak::mem_fn(control, &Control::working_directory),
                                                                           rak::mem_fn(control, &Control::set_working_directory)));

  variables->insert("load",                  new utils::VariableStringSlot(rak::value_fn(std::string()), rak::bind_ptr_fn(&apply_load, c)));
  variables->insert("load_verbose",          new utils::VariableStringSlot(rak::value_fn(std::string()), rak::bind_ptr_fn(&apply_load_verbose, c)));
  variables->insert("load_start",            new utils::VariableStringSlot(rak::value_fn(std::string()), rak::bind_ptr_fn(&apply_load_start, c)));
  variables->insert("load_start_verbose",    new utils::VariableStringSlot(rak::value_fn(std::string()), rak::bind_ptr_fn(&apply_load_start_verbose, c)));

  variables->insert("start_tied",            new utils::VariableVoidSlot(rak::bind_ptr_fn(&apply_start_tied, c)));
  variables->insert("stop_untied",           new utils::VariableVoidSlot(rak::bind_ptr_fn(&apply_stop_untied, c)));
  variables->insert("close_untied",          new utils::VariableVoidSlot(rak::bind_ptr_fn(&apply_close_untied, c)));
  variables->insert("remove_untied",         new utils::VariableVoidSlot(rak::bind_ptr_fn(&apply_remove_untied, c)));

  variables->insert("close_low_diskspace",   new utils::VariableValueSlot(rak::value_fn(int64_t()), rak::bind_ptr_fn(&apply_close_low_diskspace, c)));
  variables->insert("stop_on_ratio",         new utils::VariableStringSlot(rak::value_fn(std::string()), rak::bind_ptr_fn(&apply_stop_on_ratio, c)));

  variables->insert("enable_trackers",       new utils::VariableStringSlot(rak::value_fn(std::string()), rak::bind_ptr_fn(&apply_enable_trackers, c)));
  variables->insert("encoding_list",         new utils::VariableStringSlot(rak::value_fn(std::string()), rak::bind_ptr_fn(&apply_encoding_list, c)));

  variables->insert("encryption",            new utils::VariableStringSlot(rak::value_fn(std::string()), rak::ptr_fn(&apply_encryption)));
  variables->insert("handshake_log",         new utils::VariableBool(false));
}
