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

class xmlrpc_error : public torrent::base_error {
public:
  xmlrpc_error(xmlrpc_env* env) : m_type(env->fault_code), m_msg(env->fault_string) {}
  xmlrpc_error(int type, char* msg) : m_type(type), m_msg(msg) {}
  virtual ~xmlrpc_error() throw() {}

  virtual int         type() const throw() { return m_type; }
  virtual const char* what() const throw() { return m_msg; }

private:
  int                 m_type;
  char*               m_msg;
};

torrent::Object xmlrpc_to_object(xmlrpc_env* env, xmlrpc_value* value, int callType = 0, rpc::target_type* target = NULL);

inline torrent::Object
xmlrpc_list_entry_to_object(xmlrpc_env* env, xmlrpc_value* src, int index) {
  xmlrpc_value* tmp;
  xmlrpc_array_read_item(env, src, index, &tmp);

  if (env->fault_occurred)
    throw xmlrpc_error(env);

  torrent::Object obj = xmlrpc_to_object(env, tmp);
  xmlrpc_DECREF(tmp);

  return obj;
}

int64_t
xmlrpc_list_entry_to_value(xmlrpc_env* env, xmlrpc_value* src, int index) {
  xmlrpc_value* tmp;
  xmlrpc_array_read_item(env, src, index, &tmp);

  if (env->fault_occurred)
    throw xmlrpc_error(env);

  switch (xmlrpc_value_type(tmp)) {
  case XMLRPC_TYPE_INT:
    int v;
    xmlrpc_read_int(env, tmp, &v);
    xmlrpc_DECREF(tmp);
    return v;

#ifdef XMLRPC_HAVE_I8
  case XMLRPC_TYPE_I8:
    long long v2;
    xmlrpc_read_i8(env, tmp, &v2);
    xmlrpc_DECREF(tmp);
    return v2;
#endif

  case XMLRPC_TYPE_STRING:
  {
    const char* str;
    xmlrpc_read_string(env, tmp, &str);

    if (env->fault_occurred)
      throw xmlrpc_error(env);

    const char* end = str;
    int64_t v3 = ::strtoll(str, (char**)&end, 0);

    ::free((void*)str);

    if (*str == '\0' || *end != '\0')
      throw xmlrpc_error(XMLRPC_TYPE_ERROR, "Invalid index.");

    return v3;
  }

  default:
    xmlrpc_DECREF(tmp);
    throw xmlrpc_error(XMLRPC_TYPE_ERROR, "Invalid type found.");
  }
}

// Consider making a helper function that creates a target_type from a
// torrent::Object, then we can just use xmlrpc_to_object.
rpc::target_type
xmlrpc_to_target(xmlrpc_env* env, xmlrpc_value* value) {
  rpc::target_type target;

  switch (xmlrpc_value_type(value)) {
  case XMLRPC_TYPE_STRING:
  {
    const char* str;
    xmlrpc_read_string(env, value, &str);

    if (env->fault_occurred)
      throw xmlrpc_error(env);

    if (std::strlen(str) == 0) {
      // When specifying void, we require a zero-length string.
      ::free((void*)str);
      return rpc::make_target();

    } else if (std::strlen(str) < 40) {
      ::free((void*)str);
      throw xmlrpc_error(XMLRPC_TYPE_ERROR, "Unsupported target type found.");
    }

    core::Download* download = xmlrpc.get_slot_find_download()(str);

    if (download == NULL) {
      ::free((void*)str);
      throw xmlrpc_error(XMLRPC_TYPE_ERROR, "Could not find info-hash.");
    }

    if (std::strlen(str) == 40) {
      ::free((void*)str);
      return rpc::make_target(download);
    }

    if (std::strlen(str) < 42 || str[40] != ':') {
      ::free((void*)str);
      throw xmlrpc_error(XMLRPC_TYPE_ERROR, "Unsupported target type found.");
    }

    // Files:    "<hash>:f<index>"
    // Trackers: "<hash>:t<index>"

    int index;
    const char* end;

    switch (str[41]) {
    case 'f':
      end = str + 42;
      index = ::strtol(str + 42, (char**)&end, 0);

      if (*str == '\0' || *end != '\0')
        throw xmlrpc_error(XMLRPC_TYPE_ERROR, "Invalid index.");

      target = rpc::make_target(XmlRpc::call_file, xmlrpc.get_slot_find_file()(download, index));
      break;

    case 't':
      end = str + 42;
      index = ::strtol(str + 42, (char**)&end, 0);

      if (*str == '\0' || *end != '\0')
        throw xmlrpc_error(XMLRPC_TYPE_ERROR, "Invalid index.");

      target = rpc::make_target(XmlRpc::call_file, xmlrpc.get_slot_find_tracker()(download, index));
      break;

    default:
      ::free((void*)str);
      throw xmlrpc_error(XMLRPC_TYPE_ERROR, "Unsupported target type found.");
    }

    ::free((void*)str);

    // Check if the target pointer is NULL.
    if (target.second == NULL)
      throw xmlrpc_error(XMLRPC_TYPE_ERROR, "Invalid index.");

    return target;
  }

  default:
    return rpc::make_target();
  }
}

rpc::target_type
xmlrpc_to_index_type(int index, int callType, core::Download* download) {
  void* result;

  switch (callType) {
  case XmlRpc::call_file:    result = xmlrpc.get_slot_find_file()(download, index); break;
  case XmlRpc::call_tracker: result = xmlrpc.get_slot_find_tracker()(download, index); break;
  default: result = NULL; break;
  }

  if (result == NULL)
    throw xmlrpc_error(XMLRPC_TYPE_ERROR, "Invalid index.");
      
  return rpc::make_target(callType, result);
}

torrent::Object
xmlrpc_to_object(xmlrpc_env* env, xmlrpc_value* value, int callType, rpc::target_type* target) {
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

    if (callType != XmlRpc::call_generic) {
      // When the call type is not supposed to be void, we'll try to
      // convert it to a command target. It's not that important that
      // it is converted to the right type here, as an mismatch will
      // be caught when executing the command.
      *target = xmlrpc_to_target(env, value);
      return torrent::Object();

    } else {
      const char* valueString;
      xmlrpc_read_string(env, value, &valueString);

      if (env->fault_occurred)
        throw xmlrpc_error(env);

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
      throw xmlrpc_error(env);

    torrent::Object result = torrent::Object(std::string(valueString, valueSize));

    // Urgh, seriously?
    ::free((void*)valueString);
    return result;
  }

  case XMLRPC_TYPE_ARRAY:
  {
    unsigned int current = 0;
    unsigned int last = xmlrpc_array_size(env, value);

    if (env->fault_occurred)
      throw xmlrpc_error(env);

    if (callType != XmlRpc::call_generic) {
      if (last < 1)
        throw xmlrpc_error(XMLRPC_TYPE_ERROR, "Too few arguments.");

      xmlrpc_value* tmp;
      xmlrpc_array_read_item(env, value, current++, &tmp);

      if (env->fault_occurred)
        throw xmlrpc_error(env);

      *target = xmlrpc_to_target(env, tmp);
      xmlrpc_DECREF(tmp);

      if (env->fault_occurred)
        throw xmlrpc_error(env);

      if (target->first == XmlRpc::call_download &&
          (callType == XmlRpc::call_file || callType == XmlRpc::call_tracker)) {
        // If we have a download target and the call type requires
        // another contained type, then we try to use the next
        // parameter as the index to support old-style calls.

        if (current == last)
          throw xmlrpc_error(XMLRPC_TYPE_ERROR, "Too few arguments.");

        *target = xmlrpc_to_index_type(xmlrpc_list_entry_to_value(env, value, current++), callType, (core::Download*)target->second);
      }
    }

    if (current + 1 < last) {
      torrent::Object result = torrent::Object::create_list();
      torrent::Object::list_type& listRef = result.as_list();

      while (current != last)
        listRef.push_back(xmlrpc_list_entry_to_object(env, value, current++));

      return result;

    } else if (current + 1 == last) {
      return xmlrpc_list_entry_to_object(env, value, current);

    } else {
      return torrent::Object();
    }
  }

  //     case XMLRPC_TYPE_STRUCT:
    //     case XMLRPC_TYPE_C_PTR:
    //     case XMLRPC_TYPE_NIL:
    //     case XMLRPC_TYPE_DEAD:
  default:
    throw xmlrpc_error(XMLRPC_TYPE_ERROR, "Unsupported type found.");
  }
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
    
    for (torrent::Object::list_const_iterator itr = object.as_list().begin(), last = object.as_list().end(); itr != last; itr++)
      xmlrpc_array_append_item(env, result, object_to_xmlrpc(env, *itr));

    return result;
  }

  case torrent::Object::TYPE_MAP:
  {
    xmlrpc_value* result = xmlrpc_struct_new(env);
    
    for (torrent::Object::map_const_iterator itr = object.as_map().begin(), last = object.as_map().end(); itr != last; itr++)
      xmlrpc_struct_set_value(env, result, itr->first.c_str(), object_to_xmlrpc(env, itr->second));

    return result;
  }

  default:
    return xmlrpc_int_new(env, 0);
  }
}

xmlrpc_value*
xmlrpc_call_command(xmlrpc_env* env, xmlrpc_value* args, void* voidServerInfo) {
  CommandMap::const_iterator itr = commands.find((const char*)voidServerInfo);

  if (itr == commands.end()) {
    xmlrpc_env_set_fault(env, XMLRPC_PARSE_ERROR, ("Command \"" + std::string((const char*)voidServerInfo) + "\" does not exist.").c_str());
    return NULL;
  }

  try {
    torrent::Object object;
    rpc::target_type target = rpc::make_target();

    xmlrpc_to_object(env, args, itr->second.target(), &target).swap(object);

    if (env->fault_occurred)
      return NULL;

    return object_to_xmlrpc(env, rpc::commands.call_command(itr, object, target));

  } catch (xmlrpc_error& e) {
    xmlrpc_env_set_fault(env, e.type(), e.what());
    return NULL;

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
XmlRpc::insert_command(const char* name, const char* parm, const char* doc) {
  xmlrpc_env localEnv;
  xmlrpc_env_init(&localEnv);

  xmlrpc_registry_add_method_w_doc(&localEnv, (xmlrpc_registry*)m_registry, NULL, name,
                                   &xmlrpc_call_command, const_cast<char*>(name), parm, doc);

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

int64_t
XmlRpc::size_limit() {
  return xmlrpc_limit_get(XMLRPC_XML_SIZE_LIMIT_ID);
}

void
XmlRpc::set_size_limit(uint64_t size) {
  if (size >= (64 << 20))
    throw torrent::input_error("Invalid XMLRPC limit size.");

  xmlrpc_limit_set(XMLRPC_XML_SIZE_LIMIT_ID, size);
}

#else

void XmlRpc::initialize() { throw torrent::resource_error("XMLRPC not supported."); }
void XmlRpc::cleanup() {}

void XmlRpc::insert_command(__UNUSED const char* name, __UNUSED const char* parm, __UNUSED const char* doc) {}
void XmlRpc::set_dialect(__UNUSED int dialect) {}

bool XmlRpc::process(__UNUSED const char* inBuffer, __UNUSED uint32_t length, __UNUSED slot_write slotWrite) { return false; }

int64_t XmlRpc::size_limit() { return 0; }
void    XmlRpc::set_size_limit(uint64_t size) {}

#endif

}
