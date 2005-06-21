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
// Contact:  Jari Sundell <jaris@ifi.uio.no>
//
//           Skomakerveien 33
//           3185 Skoppum, NORWAY

#include "config.h"

#include <arpa/inet.h>
#include <torrent/torrent.h>
#include <netinet/in.h>

#include "ui/control.h"
#include "option_handler_rules.h"

void receive_tracker_dump(std::istream* s);

bool
validate_ip(const std::string& arg) {
  struct in_addr addr;

  return inet_aton(arg.c_str(), &addr);
}

bool
validate_directory(const std::string& arg) {
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
validate_download_peers(int arg) {
  return arg > 0 && arg < (1 << 16);
}

bool
validate_rate(int arg) {
  return arg >= 0 && arg < (1 << 20);
}

bool
validate_read_ahead(int arg) {
  return arg >= 1 && arg < 64;
}

bool
validate_fd(int arg) {
  return arg >= 10 && arg < (1 << 16);
}

void
apply_download_min_peers(ui::Control* m, int arg) {
  m->get_core().get_download_list().slot_map_insert().insert("1_min_peers", sigc::bind(sigc::mem_fun(&core::Download::call<void, uint32_t, &torrent::Download::set_peers_min>), arg));
}

void
apply_download_max_peers(ui::Control* m, int arg) {
  m->get_core().get_download_list().slot_map_insert().insert("1_max_peers", sigc::bind(sigc::mem_fun(&core::Download::call<void, uint32_t, &torrent::Download::set_peers_max>), arg));
}

void
apply_download_max_uploads(ui::Control* m, int arg) {
  m->get_core().get_download_list().slot_map_insert().insert("1_max_uploads", sigc::bind(sigc::mem_fun(&core::Download::call<void, uint32_t, &torrent::Download::set_uploads_max>), arg));
}

void
apply_download_directory(ui::Control* m, const std::string& arg) {
  m->get_core().get_download_list().slot_map_insert().insert("1_directory", sigc::bind(sigc::mem_fun(&core::Download::set_root_directory), arg));
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
apply_max_open_files(ui::Control* m, int arg) {
  torrent::set_max_open_files(arg);
}

void
apply_bind(ui::Control* m, const std::string& arg) {
  torrent::set_bind(arg);
}

void
apply_ip(ui::Control* m, const std::string& arg) {
  torrent::set_ip(arg);
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
apply_tracker_dump(ui::Control* m, const std::string& arg) {
  if (arg == "yes")
    m->get_core().get_download_list().slot_map_insert().insert("1_tracker_dump", sigc::bind(sigc::mem_fun(&core::Download::call<sigc::connection, torrent::Download::SlotIStream, &torrent::Download::signal_tracker_dump>), sigc::ptr_fun(&receive_tracker_dump)));
  else
    m->get_core().get_download_list().slot_map_insert().erase("1_tracker_dump");
}

void
apply_check_hash(ui::Control* m, const std::string& arg) {
  if (arg == "yes")
    m->get_core().get_download_list().slot_map_finished().insert("1_check_hash", sigc::mem_fun(m->get_core(), &core::Manager::check_hash));
  else
    m->get_core().get_download_list().slot_map_finished().erase("1_check_hash");
}
