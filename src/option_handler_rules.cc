// rTorrent - BitTorrent client
// Copyright (C) 2005, Jari Sundell
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
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <rak/string_manip.h>
#include <torrent/exceptions.h>
#include <torrent/torrent.h>

#include "core/manager.h"
#include "ui/root.h"
#include "utils/directory.h"

#include "control.h"
#include "option_handler_rules.h"
#include "command_scheduler.h"
#include "command_scheduler_item.h"

void receive_tracker_dump(std::istream* s);

void
OptionHandlerInt::process(const std::string& key, const std::string& arg) {
  int a;
    
  if (std::sscanf(arg.c_str(), "%i", &a) != 1)
    throw torrent::input_error("Invalid argument for \"" + key + "\": \"" + arg + "\", must be an integer.");
    
  m_apply(m_control, a);
}

void
OptionHandlerOctal::process(const std::string& key, const std::string& arg) {
  int a;
    
  if (std::sscanf(arg.c_str(), "%o", &a) != 1)
    throw torrent::input_error("Invalid argument for \"" + key + "\": \"" + arg + "\", must be an octal.");
    
  m_apply(m_control, a);
}

void
OptionHandlerString::process(const std::string& key, const std::string& arg) {
  m_apply(m_control, arg);
}

void
apply_download_min_peers(Control* m, int arg) {
  m->core()->get_download_list().slot_map_insert()["1_min_peers"] = sigc::bind(sigc::mem_fun(&core::Download::call<void, uint32_t, &torrent::Download::set_peers_min>), arg);
}

void
apply_download_max_peers(Control* m, int arg) {
  m->core()->get_download_list().slot_map_insert()["1_max_peers"] = sigc::bind(sigc::mem_fun(&core::Download::call<void, uint32_t, &torrent::Download::set_peers_max>), arg);
}

void
apply_download_max_uploads(Control* m, int arg) {
  m->core()->get_download_list().slot_map_insert()["1_max_uploads"] = sigc::bind(sigc::mem_fun(&core::Download::call<void, uint32_t, &torrent::Download::set_uploads_max>), arg);
}

void
apply_download_directory(Control* m, const std::string& arg) {
  if (!arg.empty())
    m->core()->get_download_list().slot_map_insert()["1_directory"] = sigc::bind(sigc::mem_fun(&core::Download::set_root_directory), arg);
  else
    m->core()->get_download_list().slot_map_insert().erase("1_directory");
}

void
apply_connection_leech(Control* m, const std::string& arg) {
  core::Download::string_to_connection_type(arg);
  m->core()->get_download_list().slot_map_insert()["1_connection_leech"] = sigc::bind(sigc::mem_fun(&core::Download::set_connection_leech), arg);
}

void
apply_connection_seed(Control* m, const std::string& arg) {
  core::Download::string_to_connection_type(arg);
  m->core()->get_download_list().slot_map_insert()["1_connection_seed"] = sigc::bind(sigc::mem_fun(&core::Download::set_connection_seed), arg);
}

void
apply_global_download_rate(Control* m, int arg) {
  m->ui()->set_down_throttle(arg);
}

void
apply_global_upload_rate(Control* m, int arg) {
  m->ui()->set_up_throttle(arg);
}

void
apply_umask(Control* m, int arg) {
  umask(arg);
}

void
apply_hash_read_ahead(Control* m, int arg) {
  torrent::set_hash_read_ahead(arg << 20);
}

void
apply_hash_interval(Control* m, int arg) {
  torrent::set_hash_interval(arg * 1000);
}

void
apply_hash_max_tries(Control* m, int arg) {
  torrent::set_hash_max_tries(arg);
}

void
apply_max_open_files(Control* m, int arg) {
  torrent::set_max_open_files(arg);
}

void
apply_max_open_sockets(Control* m, int arg) {
  torrent::set_max_open_sockets(arg);
}

void
apply_bind(Control* m, const std::string& arg) {
  bool reopenListen = torrent::listen_port();

  if (reopenListen)
    torrent::listen_close();

  torrent::set_bind_address(arg);

  if (reopenListen)
    m->core()->listen_open();
}

void
apply_ip(Control* m, const std::string& arg) {
  torrent::set_local_address(arg);
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
apply_port_random(Control* m, const std::string& arg) {
  m->core()->set_port_random(arg == "yes");
}

void
apply_tracker_dump(Control* m, const std::string& arg) {
  if (arg == "yes")
    m->core()->get_download_list().slot_map_insert()["1_tracker_dump"] = sigc::bind(sigc::mem_fun(&core::Download::call<sigc::connection, torrent::Download::SlotIStream, &torrent::Download::signal_tracker_dump>), sigc::ptr_fun(&receive_tracker_dump));
  else
    m->core()->get_download_list().slot_map_insert().erase("1_tracker_dump");
}

void
apply_use_udp_trackers(Control* m, const std::string& arg) {
  if (arg == "yes")
    m->core()->get_download_list().slot_map_insert().erase("1_use_udp_trackers");
  else
    m->core()->get_download_list().slot_map_insert()["1_use_udp_trackers"] = sigc::bind(sigc::mem_fun(&core::Download::enable_udp_trackers), false);
}

void
apply_check_hash(Control* m, const std::string& arg) {
  if (arg == "yes")
    m->core()->set_check_hash(true);
  else
    m->core()->set_check_hash(false);
}

void
apply_http_proxy(Control* m, const std::string& arg) {
  m->core()->get_poll_manager()->get_http_stack()->set_http_proxy(arg);
}

void
apply_session_directory(Control* m, const std::string& arg) {
  m->core()->get_download_store().use(arg);
}

void
apply_encoding_list(Control* m, const std::string& arg) {
  torrent::encoding_list()->push_back(arg);
}

void
apply_schedule(Control* m, const std::string& arg) {
  int first;
  int interval;
  char key[21];
  char command[2048];

  if (std::sscanf(arg.c_str(), "%20[^,],%i,%i,%2047[^\n]", key, &first, &interval, command) != 4)
    throw torrent::input_error("Invalid arguments to command.");

  CommandSchedulerItem* item = *m->command_scheduler()->insert(rak::trim(std::string(key)));

  item->set_command(rak::trim(std::string(command)));
  item->set_interval(interval);

  item->enable(first);
}

void
apply_schedule_remove(Control* m, const std::string& arg) {
  m->command_scheduler()->erase(m->command_scheduler()->find(rak::trim(arg)));
}
