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

#include <torrent/exceptions.h>

#include "globals.h"
#include "control.h"
#include "command_helpers.h"

void initialize_command_dynamic();
void initialize_command_download();
void initialize_command_events();
void initialize_command_file();
void initialize_command_ip();
void initialize_command_peer();
void initialize_command_local();
void initialize_command_logging();
void initialize_command_network();
void initialize_command_groups();
void initialize_command_throttle();
void initialize_command_tracker();
void initialize_command_scheduler();
void initialize_command_ui();

void
initialize_commands() {
  initialize_command_dynamic();
  initialize_command_events();
  initialize_command_network();
  initialize_command_groups();
  initialize_command_local();
  initialize_command_logging();
  initialize_command_ui();
  initialize_command_download();
  initialize_command_file();
  initialize_command_ip();
  initialize_command_peer();
  initialize_command_throttle();
  initialize_command_tracker();
  initialize_command_scheduler();
}
