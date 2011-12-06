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

#include <fstream>
#include <rak/path.h>
#include <torrent/peer/peer_list.h>
#include <torrent/utils/log.h>
#include <torrent/utils/option_strings.h>

#include "globals.h"
#include "command_helpers.h"

void
ipv4_filter_parse(const char* address, int value) {
  uint32_t ip_values[4] = { 0, 0, 0, 0 };
  unsigned int block = rpc::ipv4_table::mask_bits;

  char ip_dot;
  int values_read;

  if ((values_read = sscanf(address, "%u%1[.]%u%1[.]%u%1[.]%u/%u",
                            ip_values + 0, &ip_dot,
                            ip_values + 1, &ip_dot,
                            ip_values + 2, &ip_dot,
                            ip_values + 3, 
                            &block)) < 2 ||

      // Make sure the dot is included.
      (values_read < 7 && values_read % 2) ||

      ip_values[0] >= 256 ||
      ip_values[1] >= 256 ||
      ip_values[2] >= 256 ||
      ip_values[3] >= 256 ||
       
      block > rpc::ipv4_table::mask_bits)
    throw torrent::input_error("Invalid address format.");

  // E.g. '10.10.' will be '10.10.0.0/16'.
  if (values_read < 7)
    block = 8 * (values_read / 2);

  torrent::PeerList::ipv4_filter()->insert((ip_values[0] << 24) + (ip_values[1] << 16) + (ip_values[2] << 8) + ip_values[3],
                                           rpc::ipv4_table::mask_bits - block, value);
}

torrent::Object
apply_ip_tables_insert_table(const std::string& args) {
  if (ip_tables.find(args) != ip_tables.end())
    throw torrent::input_error("IP table already exists.");

  ip_tables.insert(args);
  return torrent::Object();
}

torrent::Object
apply_ip_tables_get(const torrent::Object::list_type& args) {
  if (args.size() != 2)
    throw torrent::input_error("Incorrect number of arguments.");

  torrent::Object::list_const_iterator args_itr = args.begin();

  const std::string& name    = (args_itr++)->as_string();
  const std::string& address = (args_itr++)->as_string();

  // Move to a helper function, add support for addresses.
  uint32_t ip_values[4];

  if (sscanf(address.c_str(), "%u.%u.%u.%u",
             ip_values + 0, ip_values + 1, ip_values + 2, ip_values + 3) != 4)
    throw torrent::input_error("Invalid address format.");

  rpc::ip_table_list::iterator table_itr = ip_tables.find(name);

  if (table_itr == ip_tables.end())
    throw torrent::input_error("Could not find ip table.");

  return table_itr->table.at((ip_values[0] << 24) + (ip_values[1] << 16) + (ip_values[2] << 8) + ip_values[3]);
}

torrent::Object
apply_ip_tables_add_address(const torrent::Object::list_type& args) {
  if (args.size() != 3)
    throw torrent::input_error("Incorrect number of arguments.");

  torrent::Object::list_const_iterator args_itr = args.begin();

  const std::string& name      = (args_itr++)->as_string();
  const std::string& address   = (args_itr++)->as_string();
  const std::string& value_str = (args_itr++)->as_string();
  
  // Move to a helper function, add support for addresses.
  uint32_t ip_values[4];
  unsigned int block = rpc::ipv4_table::mask_bits;

  if (sscanf(address.c_str(), "%u.%u.%u.%u/%u",
             ip_values + 0, ip_values + 1, ip_values + 2, ip_values + 3, &block) < 4 ||
      block > rpc::ipv4_table::mask_bits)
    throw torrent::input_error("Invalid address format.");

  int value;

  if (value_str == "block")
    value = 1;
  else
    throw torrent::input_error("Invalid value.");

  rpc::ip_table_list::iterator table_itr = ip_tables.find(name);

  if (table_itr == ip_tables.end())
    throw torrent::input_error("Could not find ip table.");

  table_itr->table.insert((ip_values[0] << 24) + (ip_values[1] << 16) + (ip_values[2] << 8) + ip_values[3],
                          rpc::ipv4_table::mask_bits - block, value);

  return torrent::Object();
}

//
// IPv4 filter functions:
//

torrent::Object
apply_ipv4_filter_size_data() {
  return torrent::PeerList::ipv4_filter()->sizeof_data();
}

torrent::Object
apply_ipv4_filter_get(const std::string& args) {
  // Move to a helper function, add support for addresses.
  uint32_t ip_values[4];

  if (sscanf(args.c_str(), "%u.%u.%u.%u",
             ip_values + 0, ip_values + 1, ip_values + 2, ip_values + 3) != 4)
    throw torrent::input_error("Invalid address format.");

  return torrent::PeerList::ipv4_filter()->at((ip_values[0] << 24) + (ip_values[1] << 16) + (ip_values[2] << 8) + ip_values[3]);
}

torrent::Object
apply_ipv4_filter_add_address(const torrent::Object::list_type& args) {
  if (args.size() != 2)
    throw torrent::input_error("Incorrect number of arguments.");

  ipv4_filter_parse(args.front().as_string().c_str(),
                    torrent::option_find_string(torrent::OPTION_IP_FILTER, args.back().as_string().c_str()));
  return torrent::Object();
}

torrent::Object
apply_ipv4_filter_load(const torrent::Object::list_type& args) {
  if (args.size() != 2)
    throw torrent::input_error("Incorrect number of arguments.");

  torrent::Object::list_const_iterator args_itr = args.begin();

  std::fstream file(rak::path_expand(args.front().as_string()).c_str(), std::ios::in);
  
  if (!file.is_open())
    throw torrent::input_error("Could not open ip filter file: " + args.front().as_string());

  int value = torrent::option_find_string(torrent::OPTION_IP_FILTER, args.back().as_string().c_str());

  char buffer[4096];
  unsigned int lineNumber = 0;

  try {
    while (file.good() && !file.getline(buffer, 4096).fail()) {
      if (file.gcount() == 0)
        throw torrent::internal_error("parse_command_file(...) file.gcount() == 0.");

      int lineLength = file.gcount() - 1;
      // In case we are at the end of the file and the last character is
      // not a line feed, we'll just increase the read character count so 
      // that the last would also be included in option line.
      if (file.eof() && file.get() != '\n')
        lineLength++;
      
      lineNumber++;

      if (buffer[0] == '\0' || buffer[0] == '#')
        continue;

      ipv4_filter_parse(buffer, value);
    }

  } catch (torrent::input_error& e) {
    snprintf(buffer, 2048, "Error in ip filter file: %s:%u: %s", args.front().as_string().c_str(), lineNumber, e.what());

    throw torrent::input_error(buffer);
  }

  lt_log_print(torrent::LOG_CONNECTION_INFO, "Loaded %u %s address blocks (%u kb in-memory) from '%s'.",
               lineNumber,
               args.back().as_string().c_str(),
               torrent::PeerList::ipv4_filter()->sizeof_data() / 1024,
               args.front().as_string().c_str());

  return torrent::Object();
}

void
initialize_command_ip() {
  CMD2_ANY_STRING  ("ip_tables.insert_table", std::bind(&apply_ip_tables_insert_table, std::placeholders::_2));
  CMD2_ANY_LIST    ("ip_tables.get",          std::bind(&apply_ip_tables_get, std::placeholders::_2));
  CMD2_ANY_LIST    ("ip_tables.add_address",  std::bind(&apply_ip_tables_add_address, std::placeholders::_2));

  CMD2_ANY         ("ipv4_filter.size_data",   std::bind(&apply_ipv4_filter_size_data));
  CMD2_ANY_STRING  ("ipv4_filter.get",         std::bind(&apply_ipv4_filter_get, std::placeholders::_2));
  CMD2_ANY_LIST    ("ipv4_filter.add_address", std::bind(&apply_ipv4_filter_add_address, std::placeholders::_2));
  CMD2_ANY_LIST    ("ipv4_filter.load",        std::bind(&apply_ipv4_filter_load, std::placeholders::_2));
}
