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

#include "xmlrpc.h"

#include "parse_commands.h"

#include <torrent/exceptions.h>

namespace rpc {

class xmlrpc_error : public torrent::base_error {
public:
  xmlrpc_error(int type, std::string msg) : m_type(type), m_msg(msg) {}
  virtual ~xmlrpc_error() throw() {}

  virtual int         type() const throw() { return m_type; }
  virtual const char* what() const throw() { return m_msg.c_str(); }

private:
  int                 m_type;
  std::string         m_msg;
};

void
XmlRpc::object_to_target(const torrent::Object& obj, int callFlags, rpc::target_type* target) {
  if (!obj.is_string()) {
    throw torrent::input_error("invalid parameters: target must be a string");
  }
  std::string target_string = obj.as_string();
  bool require_index = (callFlags & (CommandMap::flag_tracker_target | CommandMap::flag_file_target));
  if (target_string.size() == 0 && !require_index) {
    return;
  }

  // Length of SHA1 hash is 40
  if (target_string.size() < 40) {
    throw torrent::input_error("invalid parameters: invalid target");
  }

  char type = 'd';
  std::string hash;
  std::string index;
  const auto& delim_pos = target_string.find_first_of(':', 40);
  if (delim_pos == target_string.npos ||
      delim_pos + 2 >= target_string.size()) {
	if (require_index) {
      throw torrent::input_error("invalid parameters: no index");
    }
    hash = target_string;
  } else {
    hash  = target_string.substr(0, delim_pos);
    type  = target_string[delim_pos + 1];
    index = target_string.substr(delim_pos + 2);
  }
  core::Download* download = xmlrpc.slot_find_download()(hash.c_str());

  if (download == nullptr)
    throw torrent::input_error("invalid parameters: info-hash not found");

  try {
    switch (type) {
      case 'd':
        *target = rpc::make_target(download);
        break;
      case 'f':
        *target = rpc::make_target(
          command_base::target_file,
          xmlrpc.slot_find_file()(download, std::stoi(std::string(index))));
        break;
      case 't':
        *target = rpc::make_target(
          command_base::target_tracker,
          xmlrpc.slot_find_tracker()(download, std::stoi(std::string(index))));
        break;
      case 'p': {
          if (index.size() < 40) {
            // -501 == XMLRPC_TYPE_ERROR, used here directly to avoid
            // conflicts between tinyxml2 and xmlrpc-c
            throw xmlrpc_error(-501, "Not a hash string.");
          }
          torrent::HashString hash;
          torrent::hash_string_from_hex_c_str(index.c_str(), hash);
          *target = rpc::make_target(
                                     command_base::target_peer,
                                     xmlrpc.slot_find_peer()(download, hash));
          break;
      }
      default:
        throw torrent::input_error("invalid parameters: unexpected target type");
    }
  } catch (const std::logic_error&) {
    throw torrent::input_error("invalid parameters: invalid index");
  }
}

#ifndef HAVE_XMLRPC_C
#ifndef HAVE_XMLRPC_TINYXML2

void XmlRpc::initialize() { throw torrent::resource_error("XMLRPC not supported."); }
void XmlRpc::cleanup() {}

void XmlRpc::insert_command(__UNUSED const char* name, __UNUSED const char* parm, __UNUSED const char* doc) {}
void XmlRpc::set_dialect(__UNUSED int dialect) {}

bool XmlRpc::process(__UNUSED const char* inBuffer, __UNUSED uint32_t length, __UNUSED slot_write slotWrite) { return false; }

int64_t XmlRpc::size_limit() { return 0; }
void    XmlRpc::set_size_limit(uint64_t size) {}

bool    XmlRpc::is_valid() const { return false; }

#endif
#endif

}
