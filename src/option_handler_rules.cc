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

#include <arpa/inet.h>
#include <torrent/torrent.h>
#include <netinet/in.h>

#include "utils/directory.h"
#include "ui/control.h"
#include "option_handler_rules.h"

void receive_tracker_dump(std::istream* s);

bool
validate_ip(const std::string& arg) {
  struct in_addr addr;

  return inet_aton(arg.c_str(), &addr);
}

// We consider an empty string to be valid as this allows us to
// disable options.
bool
validate_directory(const std::string& arg) {
  //return arg.empty() || utils::Directory(arg).is_valid();
  return true;
}

bool
validate_port_range(const std::string& arg) {
  int a, b;
  
  return std::sscanf(arg.c_str(), "%i-%i", &a, &b) == 2 &&
    a <= b && a > 0 && b < (1 << 16);
}

bool
validate_yes_no(const std::string& arg) {
  return arg == "yes" || arg == "no";
}

bool
validate_non_empty(const std::string& arg) {
  return !arg.empty();
}

bool
validate_download_peers(int arg) {
  return arg > 0 && arg < (1 << 16);
}

bool
validate_rate(int arg) {
  return arg >= 0 && arg < (1 << 20);
}

bool
validate_hash_read_ahead(int arg) {
  return arg >= 1 && arg < 64;
}

bool
validate_hash_interval(int arg) {
  return arg >= 1 && arg < 1000;
}

bool
validate_hash_max_tries(int arg) {
  return arg >= 1 && arg < 20;
}

bool
validate_fd(int arg) {
  return arg >= 1 && arg < (1 << 16);
}

bool
validate_throttle_interval(int arg) {
  return arg > 0 && arg < 5000;
}

void
apply_download_min_peers(ui::Control* m, int arg) {
  m->get_core().get_download_list().slot_map_insert()["1_min_peers"] = sigc::bind(sigc::mem_fun(&core::Download::call<void, uint32_t, &torrent::Download::set_peers_min>), arg);
}

void
apply_download_max_peers(ui::Control* m, int arg) {
  m->get_core().get_download_list().slot_map_insert()["1_max_peers"] = sigc::bind(sigc::mem_fun(&core::Download::call<void, uint32_t, &torrent::Download::set_peers_max>), arg);
}

void
apply_download_max_uploads(ui::Control* m, int arg) {
  m->get_core().get_download_list().slot_map_insert()["1_max_uploads"] = sigc::bind(sigc::mem_fun(&core::Download::call<void, uint32_t, &torrent::Download::set_uploads_max>), arg);
}

void
apply_download_directory(ui::Control* m, const std::string& arg) {
  if (!arg.empty())
    m->get_core().get_download_list().slot_map_insert()["1_directory"] = sigc::bind(sigc::mem_fun(&core::Download::set_root_directory), arg);
  else
    m->get_core().get_download_list().slot_map_insert().erase("1_directory");
}

void
apply_connection_leech(ui::Control* m, const std::string& arg) {
  m->get_core().get_download_list().slot_map_insert()["1_connection_leech"] = sigc::bind(sigc::mem_fun(&core::Download::set_connection_leech), arg);
}

void
apply_connection_seed(ui::Control* m, const std::string& arg) {
  m->get_core().get_download_list().slot_map_insert()["1_connection_seed"] = sigc::bind(sigc::mem_fun(&core::Download::set_connection_seed), arg);
}

void
apply_global_download_rate(ui::Control* m, int arg) {
  torrent::set_read_throttle(arg * 1024);
}

void
apply_global_upload_rate(ui::Control* m, int arg) {
  torrent::set_write_throttle(arg * 1024);
}

void
apply_hash_read_ahead(ui::Control* m, int arg) {
  torrent::set_hash_read_ahead(arg << 20);
}

void
apply_hash_interval(ui::Control* m, int arg) {
  torrent::set_hash_interval(arg * 1000);
}

void
apply_hash_max_tries(ui::Control* m, int arg) {
  torrent::set_hash_max_tries(arg);
}

void
apply_max_open_files(ui::Control* m, int arg) {
  torrent::set_max_open_files(arg);
}

void
apply_max_open_sockets(ui::Control* m, int arg) {
  torrent::set_max_open_sockets(arg);
}

void
apply_throttle_interval(ui::Control* m, int arg) {
  torrent::set_throttle_interval(arg * 1000);
}

void
apply_bind(ui::Control* m, const std::string& arg) {
  torrent::set_bind_address(arg);
}

void
apply_ip(ui::Control* m, const std::string& arg) {
  torrent::set_address(arg);
}

// The arg string *must* have been checked with validate_port_range
// first.
void
apply_port_range(ui::Control* m, const std::string& arg) {
  int a = 0, b = 0;
    
  std::sscanf(arg.c_str(), "%i-%i", &a, &b);

  m->get_core().set_port_range(a, b);
}

void
apply_port_random(ui::Control* m, const std::string& arg) {
  m->get_core().set_port_random(arg == "yes");
}

void
apply_tracker_dump(ui::Control* m, const std::string& arg) {
  if (arg == "yes")
    m->get_core().get_download_list().slot_map_insert()["1_tracker_dump"] = sigc::bind(sigc::mem_fun(&core::Download::call<sigc::connection, torrent::Download::SlotIStream, &torrent::Download::signal_tracker_dump>), sigc::ptr_fun(&receive_tracker_dump));
  else
    m->get_core().get_download_list().slot_map_insert().erase("1_tracker_dump");
}

void
apply_use_udp_trackers(ui::Control* m, const std::string& arg) {
  if (arg == "yes")
    m->get_core().get_download_list().slot_map_insert().erase("1_use_udp_trackers");
  else
    m->get_core().get_download_list().slot_map_insert()["1_use_udp_trackers"] = sigc::bind(sigc::mem_fun(&core::Download::enable_udp_trackers), false);
}

void
apply_check_hash(ui::Control* m, const std::string& arg) {
  if (arg == "yes")
    m->get_core().set_check_hash(true);
  else
    m->get_core().set_check_hash(false);
}

void
apply_session_directory(ui::Control* m, const std::string& arg) {
  m->get_core().get_download_store().use(arg);
}
