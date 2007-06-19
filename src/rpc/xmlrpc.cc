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
#include <xmlrpc-c/server.h>
#endif

#include <torrent/object.h>
#include <torrent/exceptions.h>

#include "xmlrpc.h"
#include "parse_commands.h"

namespace rpc {

XmlRpc::slot_find_download XmlRpc::m_slotFindDownload;

#ifdef HAVE_XMLRPC_C

torrent::Object
xmlrpc_to_object(xmlrpc_env* env, xmlrpc_value* value) {
  switch (xmlrpc_value_type(value)) {
  case XMLRPC_TYPE_INT:
    int v;
    xmlrpc_read_int(env, value, &v);
      
    return torrent::Object((int64_t)v);

    //     case XMLRPC_TYPE_BOOL:
    //     case XMLRPC_TYPE_DOUBLE:
    //     case XMLRPC_TYPE_DATETIME:

  case XMLRPC_TYPE_STRING:
  {
    const char* valueString;
    xmlrpc_read_string(env, value, &valueString);

    if (env->fault_occurred)
      return torrent::Object();

    torrent::Object result = torrent::Object(std::string(valueString));

    // Urgh, seriously?
    ::free((void*)valueString);
    return result;
  }

    //     case XMLRPC_TYPE_BASE64:
  case XMLRPC_TYPE_ARRAY:
  {
    torrent::Object result(torrent::Object::TYPE_LIST);
    torrent::Object::list_type& listRef = result.as_list();

    unsigned int last = xmlrpc_array_size(env, value);

    if (env->fault_occurred)
      return torrent::Object();

    // Move this into a helper function.
    for (unsigned int i = 0; i != last; i++) {
      // Need to decref.
      xmlrpc_value* tmp;
      xmlrpc_array_read_item(env, value, i, &tmp);

      if (env->fault_occurred)
        return torrent::Object();

      listRef.push_back(xmlrpc_to_object(env, tmp));
      xmlrpc_DECREF(tmp);
    }

    return result;
  }

  //     case XMLRPC_TYPE_STRUCT:
    //     case XMLRPC_TYPE_C_PTR:
    //     case XMLRPC_TYPE_NIL:
    //     case XMLRPC_TYPE_DEAD:
  default:
    xmlrpc_env_set_fault(env, XMLRPC_TYPE_ERROR, "Unsupported type found.");
    return torrent::Object();
  }
}

core::Download*
XmlRpc::xmlrpc_to_download(xmlrpc_env* env, xmlrpc_value* value) {
  core::Download* download = NULL;

  switch (xmlrpc_value_type(value)) {
  case XMLRPC_TYPE_STRING:
    const char* valueString;
    xmlrpc_read_string(env, value, &valueString);

    if (env->fault_occurred)
      return NULL;

    if (std::strlen(valueString) != 40 ||
        (download = m_slotFindDownload(valueString)) == NULL)
      xmlrpc_env_set_fault(env, XMLRPC_TYPE_ERROR, "Could not find info-hash.");

    // Urgh, seriously?
    ::free((void*)valueString);
    return download;

  default:
    xmlrpc_env_set_fault(env, XMLRPC_TYPE_ERROR, "Unsupported type found.");
    return NULL;
  }
}

// This should really be cleaned up and support for an array of
// downloads should be added.
torrent::Object
XmlRpc::xmlrpc_to_object_d(xmlrpc_env* env, xmlrpc_value* value, core::Download** download) {
  switch (xmlrpc_value_type(value)) {
  case XMLRPC_TYPE_STRING:
    *download = xmlrpc_to_download(env, value);
    return torrent::Object();

  case XMLRPC_TYPE_ARRAY:
  {
    unsigned int last = xmlrpc_array_size(env, value);

    if (env->fault_occurred || last < 1)
      return torrent::Object();

    // Need to decref.
    xmlrpc_value* tmpDownload;
    xmlrpc_array_read_item(env, value, 0, &tmpDownload);

    if (env->fault_occurred)
      return torrent::Object();

    *download = xmlrpc_to_download(env, tmpDownload);
    xmlrpc_DECREF(tmpDownload);

    if (env->fault_occurred)
      return torrent::Object();

    torrent::Object result;

    if (last > 2) {
      result = torrent::Object(torrent::Object::TYPE_LIST);
      torrent::Object::list_type& listRef = result.as_list();

      // Move this into a helper function.
      for (unsigned int i = 1; i != last; i++) {
        // Need to decref.
        xmlrpc_value* tmp;
        xmlrpc_array_read_item(env, value, i, &tmp);

        if (env->fault_occurred)
          return torrent::Object();

        listRef.push_back(xmlrpc_to_object(env, tmp));
        xmlrpc_DECREF(tmp);
      }

      return result;

    } else if (last == 2) {
      // Need to decref.
      xmlrpc_value* tmp;
      xmlrpc_array_read_item(env, value, 1, &tmp);

      if (env->fault_occurred)
        return torrent::Object();

      result = xmlrpc_to_object(env, tmp);
      xmlrpc_DECREF(tmp);
    }

    return result;
  }

  default:
    xmlrpc_env_set_fault(env, XMLRPC_TYPE_ERROR, "Unsupported type found.");
    return torrent::Object();
  }
}

xmlrpc_value*
object_to_xmlrpc(xmlrpc_env* env, const torrent::Object& object) {
  xmlrpc_value* result;
  xmlrpc_int32  tmpInt;

  switch (object.type()) {
  case torrent::Object::TYPE_VALUE:
    tmpInt = object.as_value();
    return xmlrpc_build_value(env, "i", tmpInt);

  case torrent::Object::TYPE_STRING:
    return xmlrpc_string_new(env, object.as_string().c_str());

  case torrent::Object::TYPE_LIST:
    result = xmlrpc_array_new(env);

    for (torrent::Object::list_type::const_iterator itr = object.as_list().begin(), last = object.as_list().end(); itr != last; itr++)
      xmlrpc_array_append_item(env, result, object_to_xmlrpc(env, *itr));

    return result;

  default:
    tmpInt = 0;
    return xmlrpc_build_value(env, "i", tmpInt);
  }
}

xmlrpc_value*
XmlRpc::call_command(xmlrpc_env* env, xmlrpc_value* args, void* voidServerInfo) {
  torrent::Object object = xmlrpc_to_object(env, args);

  if (env->fault_occurred)
    return NULL;

  try {
    return object_to_xmlrpc(env, rpc::call_command((const char*)voidServerInfo, object));

  } catch (torrent::local_error& e) {
    xmlrpc_env_set_fault(env, XMLRPC_PARSE_ERROR, e.what());
    return NULL;
  }
}

xmlrpc_value*
XmlRpc::call_command_d(xmlrpc_env* env, xmlrpc_value* args, void* voidServerInfo) {
  core::Download* download;
  torrent::Object object = xmlrpc_to_object_d(env, args, &download);

  if (env->fault_occurred)
    return NULL;

  try {
    return object_to_xmlrpc(env, rpc::call_command_d((const char*)voidServerInfo, download, object));

  } catch (torrent::local_error& e) {
    xmlrpc_env_set_fault(env, XMLRPC_PARSE_ERROR, e.what());
    return NULL;
  }
}

XmlRpc::XmlRpc() : m_env(new xmlrpc_env) {
  xmlrpc_env_init(m_env);
  m_registry = xmlrpc_registry_new(m_env);
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

void
XmlRpc::insert_command(const char* name, const char* parm, const char* doc, bool onDownload) {
  xmlrpc_env localEnv;
  xmlrpc_env_init(&localEnv);

  xmlrpc_registry_add_method_w_doc(&localEnv, m_registry, NULL, name,
                                   onDownload ? &XmlRpc::call_command_d : &XmlRpc::call_command,
                                   const_cast<char*>(name), parm, doc);

  if (localEnv.fault_occurred)
    throw torrent::internal_error("Fault occured while inserting xmlrpc call.");

  xmlrpc_env_clean(&localEnv);
}

#else

XmlRpc::XmlRpc() { throw torrent::resource_error("XMLRPC not supported."); }
XmlRpc::~XmlRpc() {}

void XmlRpc::insert_command(const char* name, const char* parm, const char* doc, bool onDownload) {}

bool XmlRpc::process(const char* inBuffer, uint32_t length, slot_write slotWrite) { return false; }

xmlrpc_value* XmlRpc::call_command(xmlrpc_env* env, xmlrpc_value* args, void* voidServerInfo) { return NULL; }
xmlrpc_value* call_command_d(xmlrpc_env* env, xmlrpc_value* args, void* voidServerInfo) { return NULL; }

#endif

}
