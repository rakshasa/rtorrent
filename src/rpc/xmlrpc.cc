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

#ifdef HAVE_XMLRPC_C
#include <stdlib.h>
#include <xmlrpc.h>
#include <xmlrpc_server.h>
#endif

#include <torrent/object.h>
#include <torrent/exceptions.h>

#include "xmlrpc.h"

namespace rpc {

#ifdef HAVE_XMLRPC_C

struct server_info_t {
  server_info_t(const char* command, XmlRpc::slot_call_command* callCommand) :
    m_command(command), m_callCommand(callCommand) {}

  const char*                m_command;
  XmlRpc::slot_call_command* m_callCommand;
};

xmlrpc_value*
xmlrpc_call_command(xmlrpc_env* env, xmlrpc_value* args, void* voidServerInfo) {
  torrent::Object object(torrent::Object::TYPE_LIST);
  torrent::Object::list_type& objectList = object.as_list();

  if (xmlrpc_value_type(args) != XMLRPC_TYPE_ARRAY)
    throw torrent::internal_error("xmlrpc_value_type(args) != XMLRPC_TYPE_ARRAY");

  unsigned int last = xmlrpc_array_size(env, args);

  if (env->fault_occurred)
    return NULL;

  // Move this into a helper function.
  for (unsigned int i = 0; i != last; i++) {
    xmlrpc_value* value;
    xmlrpc_array_read_item(env, args, i, &value);

    if (env->fault_occurred)
      return NULL;

    switch (xmlrpc_value_type(value)) {
    case XMLRPC_TYPE_INT:
      int v;
      xmlrpc_read_int(env, value, &v);
      
      objectList.push_back(torrent::Object((int64_t)v));
      break;

//     case XMLRPC_TYPE_BOOL:
//     case XMLRPC_TYPE_DOUBLE:
//     case XMLRPC_TYPE_DATETIME:

    case XMLRPC_TYPE_STRING:
      const char* valueString;
      xmlrpc_read_string(env, value, &valueString);

      if (env->fault_occurred)
        return NULL;

      objectList.push_back(torrent::Object(std::string(valueString)));

      // Urgh, seriously?
      ::free((void*)valueString);
      break;

//     case XMLRPC_TYPE_BASE64:
//     case XMLRPC_TYPE_ARRAY:
//     case XMLRPC_TYPE_STRUCT:
//     case XMLRPC_TYPE_C_PTR:
//     case XMLRPC_TYPE_NIL:
//     case XMLRPC_TYPE_DEAD:
    default:
      xmlrpc_env_set_fault(env, XMLRPC_TYPE_ERROR, "Unsupported type found.");
      return NULL;
    }

    if (env->fault_occurred)
      return NULL;
  }

  try {
    server_info_t* serverInfo = reinterpret_cast<server_info_t*>(voidServerInfo);

    const torrent::Object& resultObject = (*serverInfo->m_callCommand)(serverInfo->m_command, object);

    xmlrpc_value* result;
    xmlrpc_int32  tmpInt;

    switch (resultObject.type()) {
    case torrent::Object::TYPE_VALUE:
      tmpInt = resultObject.as_value();
      result = xmlrpc_build_value(env, "i", tmpInt);
      break;

    case torrent::Object::TYPE_STRING:
      result = xmlrpc_string_new(env, resultObject.as_string().c_str());
      break;

    default:
      tmpInt = 1;
      result = xmlrpc_build_value(env, "i", tmpInt);
    }

    return result;

  } catch (torrent::local_error& e) {
    xmlrpc_env_set_fault(env, XMLRPC_PARSE_ERROR, e.what());
    return NULL;
  }
}

XmlRpc::XmlRpc() : m_env(new xmlrpc_env) {
  xmlrpc_env_init(m_env);
  m_registry = xmlrpc_registry_new(m_env);

  // Add a helper function for this...

  xmlrpc_registry_add_method_w_doc(m_env, m_registry, NULL, "call.set_upload_rate", &xmlrpc_call_command, new server_info_t("upload_rate", &m_slotSet), "i:i", "");
  xmlrpc_registry_add_method_w_doc(m_env, m_registry, NULL, "call.get_upload_rate", &xmlrpc_call_command, new server_info_t("upload_rate", &m_slotGet), "i:", "");

  xmlrpc_registry_add_method_w_doc(m_env, m_registry, NULL, "call.get_directory",   &xmlrpc_call_command, new server_info_t("directory", &m_slotGet), "s:", "");
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

#else

XmlRpc::XmlRpc() { throw torrent::resource_error("XMLRPC not supported."); }
XmlRpc::~XmlRpc() {}

bool XmlRpc::process(const char* inBuffer, uint32_t length, slot_write slotWrite) { return false; }

#endif

}
