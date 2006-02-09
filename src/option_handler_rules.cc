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
#include <rak/functional.h>
#include <rak/string_manip.h>
#include <torrent/bencode.h>
#include <torrent/exceptions.h>
#include <torrent/torrent.h>

#include "core/download.h"
#include "core/manager.h"
#include "ui/root.h"
#include "utils/directory.h"
#include "utils/variable_generic.h"
#include "utils/variable_map.h"

#include "globals.h"
#include "control.h"
#include "option_handler_rules.h"
#include "command_scheduler.h"

void
apply_umask(int arg) {
  umask(arg);
}

void
apply_working_directory(const std::string& path) {
  if (chdir(path.c_str()) != 0)
    throw torrent::input_error("Could not change working directory.");
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
apply_http_proxy(Control* m, const std::string& arg) {
  m->core()->get_poll_manager()->get_http_stack()->set_http_proxy(arg);
}

void
apply_load(Control* m, const std::string& arg) {
  m->core()->try_create_download_expand(arg, false, false, true);
}

void
apply_load_start(Control* m, const std::string& arg) {
  m->core()->try_create_download_expand(arg, true, false, true);
}

void
apply_stop_untied(Control* m, __UNUSED const std::string& arg) {
  core::Manager::DListItr itr = m->core()->download_list().begin();

  while ((itr = std::find_if(itr, m->core()->download_list().end(),
			     rak::on(rak::bind2nd(std::mem_fun(&core::Download::variable_string), "tied_to_file"),
				     std::not1(std::mem_fun_ref(&std::string::empty)))))
	 != m->core()->download_list().end()) {
    rak::file_stat fs;

    if (!fs.update((*itr)->variable_string("tied_to_file"))) {
      (*itr)->variables()->set("tied_to_file", std::string());
      m->core()->stop(*itr);
    }

    ++itr;
  }
}

void
apply_remove_untied(Control* m, __UNUSED const std::string& arg) {
  core::Manager::DListItr itr = m->core()->download_list().begin();

  while ((itr = std::find_if(itr, m->core()->download_list().end(),
			     rak::on(rak::bind2nd(std::mem_fun(&core::Download::variable_string), "tied_to_file"),
				     std::not1(std::mem_fun_ref(&std::string::empty)))))
	 != m->core()->download_list().end()) {
    rak::file_stat fs;

    if (!fs.update((*itr)->variable_string("tied_to_file"))) {
      (*itr)->variables()->set("tied_to_file", std::string());
      m->core()->stop(*itr);
      itr = m->core()->erase(itr);

    } else {
      ++itr;
    }
  }
}

void
apply_encoding_list(__UNUSED Control* m, const std::string& arg) {
  torrent::encoding_list()->push_back(arg);
}

void
initialize_option_handler(Control* c) {
  utils::VariableMap* variables = control->variables();

  // Cleaned up.
  variables->insert("check_hash",            new utils::VariableAny("yes"));
  variables->insert("use_udp_trackers",      new utils::VariableAny("yes"));
  variables->insert("port_random",           new utils::VariableAny("yes"));
  variables->insert("session",               new utils::VariableSlotString<>(NULL, rak::mem_fn(&control->core()->download_store(), &core::DownloadStore::set_path)));
  variables->insert("session_lock",          new utils::VariableAny("yes"));
  variables->insert("session_on_completion", new utils::VariableAny("yes"));

  variables->insert("connection_leech",    new utils::VariableAny("leech"));
  variables->insert("connection_seed",     new utils::VariableAny("seed"));

  variables->insert("directory",           new utils::VariableAny("./"));
  variables->insert("working_directory",   new utils::VariableSlotString<>(NULL, rak::ptr_fn(&apply_working_directory)));
  variables->insert("ip",                  new utils::VariableSlotString<>(NULL, rak::ptr_fn(&torrent::set_local_address)));
  variables->insert("bind",                new utils::VariableSlotString<>(NULL, rak::mem_fn(control->core(), &core::Manager::bind)));

  variables->insert("min_peers",           new utils::VariableValue(40));
  variables->insert("max_peers",           new utils::VariableValue(100));
  variables->insert("max_uploads",         new utils::VariableValue(15));

  variables->insert("download_rate",       new utils::VariableSlotValue<uint32_t, unsigned int>(NULL, rak::mem_fn(control->ui(), &ui::Root::set_down_throttle), "%i"));
  variables->insert("upload_rate",         new utils::VariableSlotValue<uint32_t, unsigned int>(NULL, rak::mem_fn(control->ui(), &ui::Root::set_up_throttle), "%i"));

  variables->insert("hash_max_tries",      new utils::VariableSlotValue<int, uint32_t>(NULL, rak::ptr_fn(&torrent::set_hash_max_tries), "%i"));
  variables->insert("max_open_files",      new utils::VariableSlotValue<int, uint32_t>(NULL, rak::ptr_fn(&torrent::set_max_open_files), "%i"));
  variables->insert("max_open_sockets",    new utils::VariableSlotValue<int, uint32_t>(NULL, rak::ptr_fn(&torrent::set_max_open_sockets), "%i"));

  variables->insert("print",               new utils::VariableSlotString<>(NULL, rak::mem_fn(control->core(), &core::Manager::push_log)));

  variables->insert("schedule",            new utils::VariableSlotString<>(NULL, rak::mem_fn<const std::string&>(c->command_scheduler(), &CommandScheduler::parse)));
  variables->insert("schedule_remove",     new utils::VariableSlotString<>(NULL, rak::mem_fn<const std::string&>(c->command_scheduler(), &CommandScheduler::erase)));

  // Old.
  variables->insert("port_range",          new utils::VariableSlotString<>(NULL, rak::bind_ptr_fn(&apply_port_range, c)));

  variables->insert("hash_read_ahead",     new utils::VariableSlotValue<int, int>(NULL, rak::bind_ptr_fn(&apply_hash_read_ahead, c), "%i"));
  variables->insert("hash_interval",       new utils::VariableSlotValue<int, int>(NULL, rak::bind_ptr_fn(&apply_hash_interval, c), "%i"));

  variables->insert("umask",               new utils::VariableSlotValue<int, int>(NULL, rak::ptr_fn(&apply_umask), "%o"));

  variables->insert("load",                new utils::VariableSlotString<>(NULL, rak::bind_ptr_fn(&apply_load, c)));
  variables->insert("load_start",          new utils::VariableSlotString<>(NULL, rak::bind_ptr_fn(&apply_load_start, c)));
  variables->insert("stop_untied",         new utils::VariableSlotString<>(NULL, rak::bind_ptr_fn(&apply_stop_untied, c)));
  variables->insert("remove_untied",       new utils::VariableSlotString<>(NULL, rak::bind_ptr_fn(&apply_remove_untied, c)));

  variables->insert("encoding_list",       new utils::VariableSlotString<>(NULL, rak::bind_ptr_fn(&apply_encoding_list, c)));

  variables->insert("http_proxy",          new utils::VariableSlotString<>(NULL, rak::bind_ptr_fn(&apply_http_proxy, c)));
}
