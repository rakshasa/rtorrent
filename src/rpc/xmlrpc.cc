// rTorrent - BitTorrent client
// Copyright (C) 2005-2007, Jari Sundell
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

#ifdef HAVE_XMLRPC_C

torrent::Object
xmlrpc_to_object(xmlrpc_env* env, xmlrpc_value* value) {
  switch (xmlrpc_value_type(value)) {
  case XMLRPC_TYPE_INT:
    int v;
    xmlrpc_read_int(env, value, &v);
      
    return torrent::Object((int64_t)v);

#ifdef XMLRPC_HAVE_I8
  case XMLRPC_TYPE_I8:
    long long v2;
    xmlrpc_read_i8(env, value, &v2);
      
    return torrent::Object((int64_t)v2);
#endif

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

  case XMLRPC_TYPE_BASE64:
  {
    size_t      valueSize;
    const char* valueString;

    xmlrpc_read_base64(env, value, &valueSize, (const unsigned char**)&valueString);

    if (env->fault_occurred)
      return torrent::Object();

    torrent::Object result = torrent::Object(std::string(valueString, valueSize));

    // Urgh, seriously?
    ::free((void*)valueString);
    return result;
  }

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
xmlrpc_to_download(xmlrpc_env* env, xmlrpc_value* value) {
  core::Download* download = NULL;

  switch (xmlrpc_value_type(value)) {
  case XMLRPC_TYPE_STRING:
  {
    const char* str;
    xmlrpc_read_string(env, value, &str);

    if (env->fault_occurred)
      return NULL;

    if (std::strlen(str) != 40 ||
        (download = xmlrpc.get_slot_find_download()(str)) == NULL)
      xmlrpc_env_set_fault(env, XMLRPC_TYPE_ERROR, "Could not find info-hash.");

    // Urgh, seriously?
    ::free((void*)str);
    return download;
  }

  default:
    xmlrpc_env_set_fault(env, XMLRPC_TYPE_ERROR, "Unsupported type found.");
    return NULL;
  }
}

void*
xmlrpc_to_index_type(xmlrpc_env* env, xmlrpc_value* value, int callType, core::Download* download) {
  int index;

  switch (xmlrpc_value_type(value)) {
  case XMLRPC_TYPE_INT:
    xmlrpc_read_int(env, value, &index);
    break;

#ifdef XMLRPC_HAVE_I8
  case XMLRPC_TYPE_I8:
    long long v2;
    xmlrpc_read_i8(env, value, &v2);
      
    index = v2;
    break;
#endif

  case XMLRPC_TYPE_STRING:
  {
    const char* str;
    xmlrpc_read_string(env, value, &str);

    if (env->fault_occurred)
      return NULL;

    const char* end = str;
    index = ::strtol(str, (char**)&end, 0);

    ::free((void*)str);

    if (*str == '\0' || *end != '\0') {
      xmlrpc_env_set_fault(env, XMLRPC_TYPE_ERROR, "Invalid index.");
      return NULL;
    }

    break;
  }

  default:
    xmlrpc_env_set_fault(env, XMLRPC_TYPE_ERROR, "Unsupported type found.");
    return NULL;
  }

  if (env->fault_occurred)
    return NULL;
    
  void* result;

  switch (callType) {
  case XmlRpc::call_file:    result = xmlrpc.get_slot_find_file()(download, index); break;
  case XmlRpc::call_tracker: result = xmlrpc.get_slot_find_tracker()(download, index); break;
  default: result = NULL; break;
  }

  if (result == NULL)
    xmlrpc_env_set_fault(env, XMLRPC_TYPE_ERROR, "Invalid index.");
      
  return result;
}

// This should really be cleaned up and support for an array of
// downloads should be added.
torrent::Object
xmlrpc_to_object_target(xmlrpc_env* env, xmlrpc_value* value, int callType, void** target) {
  switch (xmlrpc_value_type(value)) {
  case XMLRPC_TYPE_STRING:
    if (callType != XmlRpc::call_download) {
      xmlrpc_env_set_fault(env, XMLRPC_TYPE_ERROR, "Unsupported type found.");
      break;
    }

    *target = xmlrpc_to_download(env, value);
    break;

  case XMLRPC_TYPE_ARRAY:
  {
    unsigned int current = 0;
    unsigned int last = xmlrpc_array_size(env, value);

    if (env->fault_occurred)
      break;

    if (last < 1) {
      xmlrpc_env_set_fault(env, XMLRPC_TYPE_ERROR, "Too few arguments.");
      break;
    }

    {
      xmlrpc_value* tmp;
      xmlrpc_array_read_item(env, value, current++, &tmp);

      if (env->fault_occurred)
        break;

      *target = xmlrpc_to_download(env, tmp);
      xmlrpc_DECREF(tmp);

      if (env->fault_occurred)
        break;
    }

    if (callType == XmlRpc::call_file || callType == XmlRpc::call_tracker) {
      if (current == last) {
        xmlrpc_env_set_fault(env, XMLRPC_TYPE_ERROR, "Too few arguments.");
        break;
      }

      xmlrpc_value* tmp;
      xmlrpc_array_read_item(env, value, current++, &tmp);

      if (env->fault_occurred)
        break;

      *target = xmlrpc_to_index_type(env, tmp, callType, (core::Download*)*target);
      xmlrpc_DECREF(tmp);

      if (env->fault_occurred)
        break;
    }

    torrent::Object result;

    if (current + 1 < last) {
      result = torrent::Object(torrent::Object::TYPE_LIST);
      torrent::Object::list_type& listRef = result.as_list();

      // Move this into a helper function?
      while (current != last) {
        xmlrpc_value* tmp;
        xmlrpc_array_read_item(env, value, current++, &tmp);

        if (env->fault_occurred)
          break;

        listRef.push_back(xmlrpc_to_object(env, tmp));
        xmlrpc_DECREF(tmp);
      }

      return result;

    } else if (current + 1 == last) {
      // Need to decref.
      xmlrpc_value* tmp;
      xmlrpc_array_read_item(env, value, current, &tmp);

      if (env->fault_occurred)
        break;

      result = xmlrpc_to_object(env, tmp);
      xmlrpc_DECREF(tmp);
    }

    return result;
  }

  default:
    xmlrpc_env_set_fault(env, XMLRPC_TYPE_ERROR, "Unsupported type found.");
  }

  return torrent::Object();
}

xmlrpc_value*
object_to_xmlrpc(xmlrpc_env* env, const torrent::Object& object) {
  switch (object.type()) {
  case torrent::Object::TYPE_VALUE:

#ifdef XMLRPC_HAVE_I8
    if (xmlrpc.dialect() != XmlRpc::dialect_generic)
      return xmlrpc_i8_new(env, object.as_value());
#else
    return xmlrpc_int_new(env, object.as_value());
#endif

  case torrent::Object::TYPE_STRING:
    return xmlrpc_string_new(env, object.as_string().c_str());

  case torrent::Object::TYPE_LIST:
  {
    xmlrpc_value* result = xmlrpc_array_new(env);
    
    for (torrent::Object::list_type::const_iterator itr = object.as_list().begin(), last = object.as_list().end(); itr != last; itr++)
      xmlrpc_array_append_item(env, result, object_to_xmlrpc(env, *itr));

    return result;
  }

  case torrent::Object::TYPE_MAP:
  {
    xmlrpc_value* result = xmlrpc_struct_new(env);
    
    for (torrent::Object::map_type::const_iterator itr = object.as_map().begin(), last = object.as_map().end(); itr != last; itr++)
      xmlrpc_struct_set_value(env, result, itr->first.c_str(), object_to_xmlrpc(env, itr->second));

    return result;
  }

  default:
    return xmlrpc_int_new(env, 0);
  }
}

xmlrpc_value*
xmlrpc_call_command(xmlrpc_env* env, xmlrpc_value* args, void* voidServerInfo) {
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
xmlrpc_call_command_d(xmlrpc_env* env, xmlrpc_value* args, void* voidServerInfo) {
  core::Download* download = NULL;
  torrent::Object object = xmlrpc_to_object_target(env, args, XmlRpc::call_download, (void**)&download);

  if (env->fault_occurred)
    return NULL;

  try {
    return object_to_xmlrpc(env, rpc::call_command_d((const char*)voidServerInfo, download, object));

  } catch (torrent::local_error& e) {
    xmlrpc_env_set_fault(env, XMLRPC_PARSE_ERROR, e.what());
    return NULL;
  }
}

xmlrpc_value*
xmlrpc_call_command_f(xmlrpc_env* env, xmlrpc_value* args, void* voidServerInfo) {
  torrent::File*  file = NULL;
  torrent::Object object = xmlrpc_to_object_target(env, args, XmlRpc::call_file, (void**)&file);

  if (env->fault_occurred)
    return NULL;

  try {
    return object_to_xmlrpc(env, rpc::commands.call_command_f((const char*)voidServerInfo, file, object));

  } catch (torrent::local_error& e) {
    xmlrpc_env_set_fault(env, XMLRPC_PARSE_ERROR, e.what());
    return NULL;
  }
}

xmlrpc_value*
xmlrpc_call_command_t(xmlrpc_env* env, xmlrpc_value* args, void* voidServerInfo) {
  torrent::Tracker* tracker = NULL;
  torrent::Object   object = xmlrpc_to_object_target(env, args, XmlRpc::call_tracker, (void**)&tracker);

  if (env->fault_occurred)
    return NULL;

  try {
    return object_to_xmlrpc(env, rpc::commands.call_command_t((const char*)voidServerInfo, tracker, object));

  } catch (torrent::local_error& e) {
    xmlrpc_env_set_fault(env, XMLRPC_PARSE_ERROR, e.what());
    return NULL;
  }
}

void
XmlRpc::initialize() {
#ifndef XMLRPC_HAVE_I8
  m_dialect = dialect_generic;
#endif
  
  m_env = new xmlrpc_env;

  xmlrpc_env_init((xmlrpc_env*)m_env);
  m_registry = xmlrpc_registry_new((xmlrpc_env*)m_env);
}

void
XmlRpc::cleanup() {
  if (!is_valid())
    return;

  xmlrpc_registry_free((xmlrpc_registry*)m_registry);
  xmlrpc_env_clean((xmlrpc_env*)m_env);
  delete (xmlrpc_env*)m_env;
}

bool
XmlRpc::process(const char* inBuffer, uint32_t length, slot_write slotWrite) {
  xmlrpc_env localEnv;
  xmlrpc_env_init(&localEnv);

  xmlrpc_mem_block* memblock = xmlrpc_registry_process_call(&localEnv, (xmlrpc_registry*)m_registry, NULL, inBuffer, length);

  bool result = slotWrite((const char*)xmlrpc_mem_block_contents(memblock),
                          xmlrpc_mem_block_size(memblock));

  xmlrpc_mem_block_free(memblock);
  xmlrpc_env_clean(&localEnv);
  return result;
}

void
XmlRpc::insert_command(const char* name, const char* parm, const char* doc, int call) {
  xmlrpc_env localEnv;
  xmlrpc_env_init(&localEnv);

  xmlrpc_value* (*callSlot)(xmlrpc_env*, xmlrpc_value*, void*);

  switch (call) {
  case call_download: callSlot = &xmlrpc_call_command_d; break;
  case call_file:     callSlot = &xmlrpc_call_command_f; break;
  case call_tracker:  callSlot = &xmlrpc_call_command_t; break;
  default:            callSlot = &xmlrpc_call_command; break;
  }

  xmlrpc_registry_add_method_w_doc(&localEnv, (xmlrpc_registry*)m_registry, NULL, name,
                                   callSlot, const_cast<char*>(name), parm, doc);

  if (localEnv.fault_occurred)
    throw torrent::internal_error("Fault occured while inserting xmlrpc call.");

  xmlrpc_env_clean(&localEnv);
}

void
XmlRpc::set_dialect(int dialect) {
  if (!is_valid())
    throw torrent::input_error("Cannot select XMLRPC dialect before it is initialized.");

  xmlrpc_env localEnv;
  xmlrpc_env_init(&localEnv);

  switch (dialect) {
  case dialect_generic:
    break;

#ifdef XMLRPC_HAVE_I8
  case dialect_i8:
    xmlrpc_registry_set_dialect(&localEnv, (xmlrpc_registry*)m_registry, xmlrpc_dialect_i8);
    break;

  case dialect_apache:
    xmlrpc_registry_set_dialect(&localEnv, (xmlrpc_registry*)m_registry, xmlrpc_dialect_apache);
    break;
#endif

  default:
    xmlrpc_env_clean(&localEnv);
    throw torrent::input_error("Unsupported XMLRPC dialect selected.");
  }

  if (localEnv.fault_occurred) {
    xmlrpc_env_clean(&localEnv);
    throw torrent::input_error("Unsupported XMLRPC dialect selected.");
  }

  xmlrpc_env_clean(&localEnv);
  m_dialect = dialect;
}

#else

void XmlRpc::initialize() { throw torrent::resource_error("XMLRPC not supported."); }
void XmlRpc::cleanup() {}

void XmlRpc::insert_command(__UNUSED const char* name, __UNUSED const char* parm, __UNUSED const char* doc, __UNUSED int call) {}
void XmlRpc::set_dialect(__UNUSED int dialect) {}

bool XmlRpc::process(__UNUSED const char* inBuffer, __UNUSED uint32_t length, __UNUSED slot_write slotWrite) { return false; }

#endif

}
