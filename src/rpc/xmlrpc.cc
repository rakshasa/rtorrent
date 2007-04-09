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

#include <xmlrpc.h>
#include <xmlrpc_server.h>

#include "xmlrpc.h"

namespace rpc {

xmlrpc_value*
xmlrpc_test_add(xmlrpc_env* env, xmlrpc_value* value, void* serverInfo) {
  // Grabbed from the XMLRPC-C examples.
  xmlrpc_int32 x, y, z;

  /* Parse our argument array. */
  xmlrpc_parse_value(env, value, "(ii)", &x, &y);
  if (env->fault_occurred)
    return NULL;

  /* Add our two numbers. */
  z = x + y;

  /* Return our result. */
  return xmlrpc_build_value(env, "i", z);
}

XmlRpc::XmlRpc() : m_env(new xmlrpc_env) {
  xmlrpc_env_init(m_env);

  m_registry = xmlrpc_registry_new(m_env);

  // Move this out.
  xmlrpc_registry_add_method_w_doc(m_env, m_registry, NULL, "my.test", &xmlrpc_test_add, new int, "i:ii", "Not much.");
}

XmlRpc::~XmlRpc() {
  xmlrpc_registry_free(m_registry);
  xmlrpc_env_clean(m_env);
  delete m_env;
}

bool
XmlRpc::process(const char* inBuffer, uint32_t length, slot_write slotWrite) {
  xmlrpc_env localEnv;
  xmlrpc_env_init(&localEnv);

  xmlrpc_mem_block* memblock = xmlrpc_registry_process_call(&localEnv, m_registry, NULL, inBuffer, length);

  bool result = slotWrite((const char*)xmlrpc_mem_block_contents(memblock),
                          xmlrpc_mem_block_size(memblock));

  xmlrpc_mem_block_free(memblock);
  xmlrpc_env_clean(&localEnv);
  return result;
}

}
