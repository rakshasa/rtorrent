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
// Contact:  Jari Sundell <sundell.software@gmail.com>


#include "config.h"

#include <fstream>
#include <rak/path.h>
#include <torrent/peer/peer_list.h>
#include <torrent/utils/log.h>
#include <torrent/utils/option_strings.h>

#include "globals.h"
#include "command_helpers.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

bool ipv4_range_parse(const char* address, uint32_t* address_start, uint32_t* address_end);

torrent::Object
apply_ip_tables_insert_table(const std::string& args) {
  if (ip_tables.find(args) != ip_tables.end())
    throw torrent::input_error("IP table already exists.");

  ip_tables.insert(args);
  return torrent::Object();
}

torrent::Object
apply_ip_tables_size_data(const std::string& args) {
  rpc::ip_table_list::const_iterator itr = ip_tables.find(args);

  if (itr != ip_tables.end())
    throw torrent::input_error("IP table does not exist.");

  uint32_t size = itr->table.sizeof_data();
  return size;
}

torrent::Object
apply_ip_tables_get(const torrent::Object::list_type& args) {
  if (args.size() != 2)
    throw torrent::input_error("Incorrect number of arguments.");

  torrent::Object::list_const_iterator args_itr = args.begin();

  const std::string& name    = (args_itr++)->as_string();
  const std::string& address = (args_itr++)->as_string();
  uint32_t address_start;
  uint32_t address_end;

  rpc::ip_table_list::iterator table_itr = ip_tables.find(name);

  if (table_itr == ip_tables.end())
    throw torrent::input_error("Could not find ip table.");

  if (!ipv4_range_parse(address.c_str(), &address_start, &address_end)) 
    throw torrent::input_error("Invalid address format.");

  if(!table_itr->table.defined(address_start, address_end))
    throw torrent::input_error("No value defined for specified IP(s).");

  return table_itr->table.at(address_start, address_end);
}

torrent::Object
apply_ip_tables_add_address(const torrent::Object::list_type& args) {
  if (args.size() != 3)
    throw torrent::input_error("Incorrect number of arguments.");

  torrent::Object::list_const_iterator args_itr = args.begin();

  const std::string& name      = (args_itr++)->as_string();
  const std::string& address   = (args_itr++)->as_string();
  const std::string& value_str = (args_itr++)->as_string();

  int value;

  if (value_str == "block")
    value = 1;
  else
    throw torrent::input_error("Invalid value.");

  rpc::ip_table_list::iterator table_itr = ip_tables.find(name);

  if (table_itr == ip_tables.end())
    throw torrent::input_error("Could not find ip table.");

  uint32_t address_start;
  uint32_t address_end;

  if (ipv4_range_parse(address.c_str(), &address_start, &address_end))
    table_itr->table.insert(address_start, address_end, value);
  else
    throw torrent::input_error("Invalid address format.");

  return torrent::Object();
}

//
// IPv4 filter functions:
//

///////////////////////////////////////////////////////////
// IPV4_RANGE_PARSE parses string into an ip range
//
// should be compatible with lines in p2p files
// everything in address before colon is ignored
//
// ip range can be single ip in which case start=end
// ip range can be cidr notation a.b.c.d/e
// ip range can be explicit range like in p2p line a.b.c.d-w.x.y.z 
//
// returns false if line does not contain valid ip or ip range
// address_start and address_end will contain start and end ip
//
// addresses parsed are returned in host byte order
// to get network byte order call htonl(address)
///////////////////////////////////////////////////////////
bool
ipv4_range_parse(const char* address, uint32_t* address_start, uint32_t* address_end) {
  // same length as buffer used to do reads so no worries about overflow
  char address_copy[4096];
  bool valid = false;
  char address_start_str[20];
  int  address_start_index=0;
  struct sockaddr_in sa_start;
  *address_start=0;
  *address_end=0;

  // get rid of everything after '#' comments
  // copy everything up to '#'  to address_copy and work from there
  while(address[address_start_index] != '#' && address[address_start_index] != '\r' &&
        address[address_start_index] != '\n' && address[address_start_index] != '\0' &&
        address_start_index < 4096 ) {

    address_copy[address_start_index] = address[address_start_index];
    address_start_index++;
  }

  address_copy[address_start_index] = '\0';
  address_start_index=0;

  // skip everything up to and including last ':' character and whitespace
  const char* addr = strrchr(address_copy, ':');

  addr = (addr == NULL) ? address_copy : addr + 1;

  while(addr[0] == ' ' || addr[0] == '\t')
    addr++;

  while(((addr[0] >= '0' && addr[0] <= '9') || addr[0] == '.') && address_start_index < 19) {
    address_start_str[address_start_index] = addr[0];
    address_start_index++;
    addr++;
  }

  address_start_str[address_start_index] = '\0';

  if(strchr(addr, '-') != NULL) {
    // explicit range
    char address_end_str[20];
    int address_end_index=0;
    struct sockaddr_in sa_end;

    while(addr[0] == '-' || addr[0] == ' ' || addr[0] == '\t')
      addr++;

    while(((addr[0] >= '0' && addr[0] <= '9') || addr[0] == '.') && address_end_index < 19) {
      address_end_str[address_end_index] = addr[0];
      address_end_index++;
      addr++;
    }

    address_end_str[address_end_index] = '\0';

    if(inet_pton(AF_INET, address_start_str, &(sa_start.sin_addr)) != 0 && inet_pton(AF_INET, address_end_str, &(sa_end.sin_addr)) != 0) {
      *address_start = ntohl(sa_start.sin_addr.s_addr);
      *address_end = ntohl(sa_end.sin_addr.s_addr);

      if(*address_start <= *address_end)
        valid=true;
    }
  } else if(strchr(addr, '/') != NULL) {
    // cidr range
    char mask_bits_str[20];
    int mask_bits_index=0;
    uint32_t mask_bits;

    while(addr[0] == '/' || addr[0] == ' ' || addr[0] == '\t')
      addr++;

    while( (addr[0] >= '0' && addr[0] <= '9') && mask_bits_index < 19) {
      mask_bits_str[mask_bits_index] = addr[0];
      mask_bits_index++;
      addr++;
    }

    mask_bits_str[mask_bits_index] = '\0';

    if(inet_pton(AF_INET, address_start_str, &(sa_start.sin_addr)) != 0 && sscanf(mask_bits_str, "%u", &mask_bits) != 0) {
      if(mask_bits <=32) {
        uint32_t ip=ntohl(sa_start.sin_addr.s_addr);
        uint32_t mask=0;
        uint32_t end_mask=0;

        mask = (~mask) << (32-mask_bits);
        *address_start = ip & mask;
        end_mask = (~end_mask) >> mask_bits;
        *address_end = (ip & mask) | end_mask;

        valid=true;
      }
    }
  } else {
    // single ip
    if(inet_pton(AF_INET, address_start_str, &(sa_start.sin_addr)) != 0) {
      *address_start = ntohl(sa_start.sin_addr.s_addr);
      *address_end = *address_start;

      valid=true;
    }
  }

  return valid;
}

///////////////////////////////////////////////////////////
// IPV4_FILTER_PARSE
//
// should now be compatible with lines in p2p files
//
// addresses in table MUST be in host byte order
// ntohl is called after parsing ip address(es)
///////////////////////////////////////////////////////////
void
ipv4_filter_parse(const char* address, int value) {
  uint32_t address_start;
  uint32_t address_end;

  if (ipv4_range_parse(address, &address_start, &address_end) ) {
    torrent::PeerList::ipv4_filter()->insert(address_start, address_end, value);

    char start_str[INET_ADDRSTRLEN];
    char end_str[INET_ADDRSTRLEN];
    uint32_t net_start = htonl(address_start);
    uint32_t net_end = htonl(address_end);

    inet_ntop(AF_INET, &net_start, start_str, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &net_end, end_str, INET_ADDRSTRLEN);

    lt_log_print(torrent::LOG_CONNECTION_FILTER, "Adding ip filter for %s-%s.", start_str, end_str);
  }
}

torrent::Object
apply_ipv4_filter_size_data() {
  return torrent::PeerList::ipv4_filter()->sizeof_data();
}

torrent::Object
apply_ipv4_filter_get(const std::string& args) {
  uint32_t address_start;
  uint32_t address_end;

  if (!ipv4_range_parse(args.c_str(), &address_start, &address_end)) 
    throw torrent::input_error("Invalid address format.");

  if(!torrent::PeerList::ipv4_filter()->defined(address_start, address_end))
    throw torrent::input_error("No value defined for specified IP(s).");

  return torrent::PeerList::ipv4_filter()->at(address_start, address_end);
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

  std::string filename = args.front().as_string();
  std::string value_name = args.back().as_string();
  int value = torrent::option_find_string(torrent::OPTION_IP_FILTER, value_name.c_str());

  std::fstream file(rak::path_expand(filename).c_str(), std::ios::in);
  
  if (!file.is_open())
    throw torrent::input_error("Could not open ip filter file: " + filename);

  unsigned int lineNumber = 0;
  char buffer[4096];

  try {
    while (file.good() && !file.getline(buffer, 4096).fail()) {
      if (file.gcount() == 0)
        throw torrent::internal_error("parse_command_file(...) file.gcount() == 0.");

      lineNumber++;

      if (buffer[0] == '\0' || buffer[0] == '#')
        continue;

      ipv4_filter_parse(buffer, value);
    }

  } catch (torrent::input_error& e) {
    snprintf(buffer, 2048, "Error in ip filter file: %s:%u: %s", filename.c_str(), lineNumber, e.what());

    throw torrent::input_error(buffer);
  }

  lt_log_print(torrent::LOG_CONNECTION_FILTER, "loaded %u %s address blocks (%u kb in-memory) from '%s'",
               lineNumber,
               value_name.c_str(),
               torrent::PeerList::ipv4_filter()->sizeof_data() / 1024,
               filename.c_str());

  return torrent::Object();
}

torrent::Object
apply_ipv4_filter_dump() {
  torrent::Object raw_result = torrent::Object::create_list();
  torrent::Object::list_type& result = raw_result.as_list();

  torrent::ipv4_table::range_map_type range_map = torrent::PeerList::ipv4_filter()->range_map;
  torrent::ipv4_table::range_map_type::iterator iter = range_map.begin();

  while(iter != range_map.end()) {
    char buffer[64];
    uint32_t address_start = iter->first;
    uint32_t address_end = (iter->second).first;
    int value = (iter->second).second;

    char start_str[INET_ADDRSTRLEN];
    char end_str[INET_ADDRSTRLEN];
    uint32_t net_start = htonl(address_start);
    uint32_t net_end = htonl(address_end);

    inet_ntop(AF_INET, &net_start, start_str, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &net_end, end_str, INET_ADDRSTRLEN);

    snprintf(buffer, 64, "%s-%s %s", start_str, end_str, torrent::option_as_string(torrent::OPTION_IP_FILTER, value));

    result.push_back((std::string)buffer);

    iter++;
  }

  return raw_result;
}

void
initialize_command_ip() {
  CMD2_ANY_STRING  ("ip_tables.insert_table",  std::bind(&apply_ip_tables_insert_table, std::placeholders::_2));
  CMD2_ANY_STRING  ("ip_tables.size_data",     std::bind(&apply_ip_tables_size_data, std::placeholders::_2));
  CMD2_ANY_LIST    ("ip_tables.get",           std::bind(&apply_ip_tables_get, std::placeholders::_2));
  CMD2_ANY_LIST    ("ip_tables.add_address",   std::bind(&apply_ip_tables_add_address, std::placeholders::_2));

  CMD2_ANY         ("ipv4_filter.size_data",   std::bind(&apply_ipv4_filter_size_data));
  CMD2_ANY_STRING  ("ipv4_filter.get",         std::bind(&apply_ipv4_filter_get, std::placeholders::_2));
  CMD2_ANY_LIST    ("ipv4_filter.add_address", std::bind(&apply_ipv4_filter_add_address, std::placeholders::_2));
  CMD2_ANY_LIST    ("ipv4_filter.load",        std::bind(&apply_ipv4_filter_load, std::placeholders::_2));
  CMD2_ANY_LIST    ("ipv4_filter.dump",        std::bind(&apply_ipv4_filter_dump));
}
