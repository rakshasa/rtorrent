#include "config.h"

#ifdef HAVE_XMLRPC_C
#ifdef HAVE_XMLRPC_TINYXML2
#error HAVE_XMLRPC_C and HAVE_XMLRPC_TINYXML2 cannot be used together. Please choose only one
#endif

#include <cctype>
#include <string>
#include <stdlib.h>
#include <torrent/object.h>
#include <torrent/exceptions.h>
#include <torrent/utils/string_manip.h>
#include <xmlrpc-c/server.h>

#include "rpc_manager.h"
#include "xmlrpc.h"
#include "parse_commands.h"
#include "utils/functional.h"

namespace rpc {

class xmlrpc_error_c : public torrent::base_error {
public:
  xmlrpc_error_c(xmlrpc_env* env) : m_type(env->fault_code), m_msg(env->fault_string) {}
  xmlrpc_error_c(int type, const char* msg) : m_type(type), m_msg(msg) {}
  virtual ~xmlrpc_error_c() throw() {}

  virtual int         type() const throw() { return m_type; }
  virtual const char* what() const throw() { return m_msg; }

private:
  int                 m_type;
  const char*         m_msg;
};

torrent::Object xmlrpc_to_object(xmlrpc_env* env, xmlrpc_value* value, int call_type = 0, rpc::target_type* target = NULL, std::function<void()>* deleter = NULL);

inline torrent::Object
xmlrpc_list_entry_to_object(xmlrpc_env* env, xmlrpc_value* src, int index) {
  xmlrpc_value* tmp;
  xmlrpc_array_read_item(env, src, index, &tmp);

  if (env->fault_occurred)
    throw xmlrpc_error_c(env);

  torrent::Object obj = xmlrpc_to_object(env, tmp);
  xmlrpc_DECREF(tmp);

  return obj;
}

int64_t
xmlrpc_list_entry_to_value(xmlrpc_env* env, xmlrpc_value* src, int index) {
  xmlrpc_value* tmp;
  xmlrpc_array_read_item(env, src, index, &tmp);

  if (env->fault_occurred)
    throw xmlrpc_error_c(env);

  switch (xmlrpc_value_type(tmp)) {
  case XMLRPC_TYPE_INT:
    int v;
    xmlrpc_read_int(env, tmp, &v);
    xmlrpc_DECREF(tmp);
    return v;

#ifdef XMLRPC_HAVE_I8
  case XMLRPC_TYPE_I8:
    xmlrpc_int64 v2;
    xmlrpc_read_i8(env, tmp, &v2);
    xmlrpc_DECREF(tmp);
    return v2;
#endif

  case XMLRPC_TYPE_STRING:
  {
    const char* str;
    xmlrpc_read_string(env, tmp, &str);

    if (env->fault_occurred)
      throw xmlrpc_error_c(env);

    const char* end = str;
    int64_t v3 = ::strtoll(str, (char**)&end, 0);

    ::free((void*)str);

    if (*str == '\0' || *end != '\0')
      throw xmlrpc_error_c(XMLRPC_TYPE_ERROR, "Invalid index.");

    return v3;
  }

  default:
    xmlrpc_DECREF(tmp);
    throw xmlrpc_error_c(XMLRPC_TYPE_ERROR, "Invalid type found.");
  }
}

std::pair<rpc::target_type, std::function<void()>>
xmlrpc_to_target(xmlrpc_env* env, xmlrpc_value* value, int call_type) {
  rpc::target_type target;
  std::function<void()> deleter = []() {};

  RpcManager::object_to_target(xmlrpc_to_object(env, value, -1, nullptr), call_type, &target, &deleter);

  return std::make_pair(target, deleter);
}

std::pair<rpc::target_type, std::function<void()>>
xmlrpc_to_index_type(int index, int call_type, core::Download* download) {
  void* result = nullptr;
  std::function<void()> deleter = []() {};

  try {

    switch (call_type) {
    case XmlRpc::call_file:
      result = rpc.slot_find_file()(download, index);
      break;

    case XmlRpc::call_tracker:
      result = new torrent::tracker::Tracker(rpc.slot_find_tracker()(download, index));
      deleter = [result]() { delete (torrent::tracker::Tracker*)result; };
      break;

    default:
      throw torrent::input_error("invalid parameters: unexpected target type");
      break;
    }

  } catch (const torrent::input_error& e) {
    throw xmlrpc_error_c(XMLRPC_TYPE_ERROR, e.what());
  }

  return std::make_pair(rpc::make_target(call_type, result), deleter);
}

torrent::Object
xmlrpc_to_object(xmlrpc_env* env, xmlrpc_value* value, int call_type, rpc::target_type* target, std::function<void()>* deleter) {
  switch (xmlrpc_value_type(value)) {
  case XMLRPC_TYPE_INT:
    int v;
    xmlrpc_read_int(env, value, &v);

    return torrent::Object((int64_t)v);

#ifdef XMLRPC_HAVE_I8
  case XMLRPC_TYPE_I8:
    xmlrpc_int64 v2;
    xmlrpc_read_i8(env, value, &v2);

    return torrent::Object((int64_t)v2);
#endif

  case XMLRPC_TYPE_STRING:

    if (call_type != XmlRpc::call_generic && target != nullptr) {
      // When the call type is not supposed to be void, we'll try to
      // convert it to a command target. It's not that important that
      // it is converted to the right type here, as an mismatch will
      // be caught when executing the command.
      std::tie(*target, *deleter) = xmlrpc_to_target(env, value, call_type);
      return torrent::Object();

    } else {
      const char* valueString;
      xmlrpc_read_string(env, value, &valueString);

      if (env->fault_occurred)
        throw xmlrpc_error_c(env);

      torrent::Object result = torrent::Object(std::string(valueString));

      ::free((void*)valueString);
      return result;
    }

  case XMLRPC_TYPE_BASE64:
  {
    size_t      valueSize;
    const char* valueString;

    xmlrpc_read_base64(env, value, &valueSize, (const unsigned char**)&valueString);

    if (env->fault_occurred)
      throw xmlrpc_error_c(env);

    torrent::Object result = torrent::Object(std::string(valueString, valueSize));

    ::free((void*)valueString);
    return result;
  }

  case XMLRPC_TYPE_ARRAY:
  {
    unsigned int current = 0;
    unsigned int last = xmlrpc_array_size(env, value);

    if (env->fault_occurred)
      throw xmlrpc_error_c(env);

    if (call_type != XmlRpc::call_generic && last != 0) {
      if (last < 1)
        throw xmlrpc_error_c(XMLRPC_TYPE_ERROR, "Too few arguments.");

      xmlrpc_value* tmp;
      xmlrpc_array_read_item(env, value, current++, &tmp);

      if (env->fault_occurred)
        throw xmlrpc_error_c(env);

      if (target != nullptr)
        std::tie(*target, *deleter) = xmlrpc_to_target(env, tmp, call_type);

      xmlrpc_DECREF(tmp);

      if (env->fault_occurred)
        throw xmlrpc_error_c(env);

      if (target != nullptr && target->first == XmlRpc::call_download &&
          (call_type == XmlRpc::call_file || call_type == XmlRpc::call_tracker)) {
        // If we have a download target and the call type requires
        // another contained type, then we try to use the next
        // parameter as the index to support old-style calls.

        if (current == last)
          throw xmlrpc_error_c(XMLRPC_TYPE_ERROR, "Too few arguments, missing index.");

        {
          std::function<void()> old_deleter, tmp_deleter;
          std::tie(*target, tmp_deleter) = xmlrpc_to_index_type(xmlrpc_list_entry_to_value(env, value, current++),
                                                                call_type,
                                                                (core::Download*)target->second);
          old_deleter.swap(*deleter);
          *deleter = [old_deleter, tmp_deleter]() {
              tmp_deleter();
              old_deleter();
            };
        }
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

  default:
    throw xmlrpc_error_c(XMLRPC_TYPE_ERROR, "Unsupported type found.");
  }
}

xmlrpc_value*
object_to_xmlrpc(xmlrpc_env* env, const torrent::Object& object) {
  switch (object.type()) {
  case torrent::Object::TYPE_VALUE:

#ifdef XMLRPC_HAVE_I8
    if (rpc.dialect() != XmlRpc::dialect_generic)
      return xmlrpc_i8_new(env, object.as_value());
#else
    return xmlrpc_int_new(env, object.as_value());
#endif

  case torrent::Object::TYPE_STRING:
  {
#ifdef XMLRPC_HAVE_I8
    // The versions that support I8 do implicit utf-8 validation.
    xmlrpc_value* result = xmlrpc_string_new(env, object.as_string().c_str());
#else
    xmlrpc_value* result{};

    // In older versions, xmlrpc-c doesn't validate the utf-8 encoding itself.
    xmlrpc_validate_utf8(env, object.as_string().c_str(), object.as_string().length());

    if (!env->fault_occurred)
      result = xmlrpc_string_new(env, object.as_string().c_str());
#endif

    if (env->fault_occurred) {
      xmlrpc_env_clean(env);
      xmlrpc_env_init(env);

      return xmlrpc_string_new(env, torrent::utils::string_with_escape_codes(object.as_string()).c_str());
    }

    return result;
  }

  case torrent::Object::TYPE_LIST:
  {
    xmlrpc_value* result = xmlrpc_array_new(env);

    for (const auto& itr : object.as_list()) {
      xmlrpc_value* item = object_to_xmlrpc(env, itr);
      xmlrpc_array_append_item(env, result, item);
      xmlrpc_DECREF(item);
    }

    return result;
  }

  case torrent::Object::TYPE_MAP:
  {
    xmlrpc_value* result = xmlrpc_struct_new(env);

    for (const auto& itr : object.as_map()) {
      xmlrpc_value* item = object_to_xmlrpc(env, itr.second);
      xmlrpc_struct_set_value(env, result, itr.first.c_str(), item);
      xmlrpc_DECREF(item);
    }

    return result;
  }

  case torrent::Object::TYPE_DICT_KEY:
  {
    xmlrpc_value* result = xmlrpc_array_new(env);

    xmlrpc_value* key_item = object_to_xmlrpc(env, object.as_dict_key());
    xmlrpc_array_append_item(env, result, key_item);
    xmlrpc_DECREF(key_item);

    if (object.as_dict_obj().is_list()) {
      for (const auto& itr : object.as_dict_obj().as_list()) {
        xmlrpc_value* item = object_to_xmlrpc(env, itr);
        xmlrpc_array_append_item(env, result, item);
        xmlrpc_DECREF(item);
      }
    } else {
      xmlrpc_value* arg_item = object_to_xmlrpc(env, object.as_dict_obj());
      xmlrpc_array_append_item(env, result, arg_item);
      xmlrpc_DECREF(arg_item);
    }

    return result;
  }

  default:
    return xmlrpc_int_new(env, 0);
  }
}

xmlrpc_value*
xmlrpc_call_command(xmlrpc_env* env, xmlrpc_value* args, void* voidServerInfo) {
  auto server_info = static_cast<const char*>(voidServerInfo);

  CommandMap::iterator itr = commands.find(server_info);

  if (itr == commands.end()) {
    xmlrpc_env_set_fault(env, XMLRPC_PARSE_ERROR, ("Command \"" + std::string(server_info) + "\" does not exist.").c_str());
    return NULL;
  }

  std::function<void()> deleter = []() {};
  utils::scope_guard    guard([&deleter]() { deleter(); });

  try {
    torrent::Object object;
    rpc::target_type target = rpc::make_target();

    if (itr->second.m_flags & CommandMap::flag_file_target)
      xmlrpc_to_object(env, args, XmlRpc::call_file, &target, &deleter).swap(object);
    else if (itr->second.m_flags & CommandMap::flag_tracker_target)
      xmlrpc_to_object(env, args, XmlRpc::call_tracker, &target, &deleter).swap(object);
    else
      xmlrpc_to_object(env, args, XmlRpc::call_any, &target, &deleter).swap(object);

    if (env->fault_occurred)
      return NULL;

    return object_to_xmlrpc(env, rpc::commands.call_command(itr, object, target));

  } catch (xmlrpc_error_c& e) {
    xmlrpc_env_set_fault(env, e.type(), e.what());
    return NULL;

  } catch (torrent::local_error& e) {
    xmlrpc_env_set_fault(env, XMLRPC_PARSE_ERROR, e.what());
    return NULL;
  }
}

bool
XmlRpc::is_valid() const {
  return m_env != NULL;
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
  xmlrpc_env local_env;
  xmlrpc_env_init(&local_env);

  xmlrpc_mem_block* memblock = xmlrpc_registry_process_call(&local_env, (xmlrpc_registry*)m_registry, NULL, inBuffer, length);

  if (local_env.fault_occurred && local_env.fault_code == XMLRPC_INTERNAL_ERROR)
    throw torrent::internal_error("Internal error in XMLRPC.");

  bool result = slotWrite((const char*)xmlrpc_mem_block_contents(memblock),
                          xmlrpc_mem_block_size(memblock));

  xmlrpc_mem_block_free(memblock);
  xmlrpc_env_clean(&local_env);
  return result;
}

void
XmlRpc::insert_command(const char* name, const char* parm, const char* doc) {
  xmlrpc_env local_env;
  xmlrpc_env_init(&local_env);

  auto stored_name = store_command_name(name);

  xmlrpc_registry_add_method_w_doc(&local_env,
                                   (xmlrpc_registry*)m_registry,
                                   nullptr,
                                   stored_name,
                                   &xmlrpc_call_command,
                                   (void*)stored_name,
                                   parm,
                                   doc);

  if (local_env.fault_occurred)
    throw torrent::internal_error("Fault occured while inserting xmlrpc call.");

  xmlrpc_env_clean(&local_env);
}

void
XmlRpc::set_dialect(int dialect) {
  if (!is_valid())
    throw torrent::input_error("Cannot select XMLRPC dialect before it is initialized.");

  xmlrpc_env local_env;
  xmlrpc_env_init(&local_env);

  switch (dialect) {
  case dialect_generic:
    break;

#ifdef XMLRPC_HAVE_I8
  case dialect_i8:
    xmlrpc_registry_set_dialect(&local_env, (xmlrpc_registry*)m_registry, xmlrpc_dialect_i8);
    break;

  case dialect_apache:
    xmlrpc_registry_set_dialect(&local_env, (xmlrpc_registry*)m_registry, xmlrpc_dialect_apache);
    break;
#endif

  default:
    xmlrpc_env_clean(&local_env);
    throw torrent::input_error("Unsupported XMLRPC dialect selected.");
  }

  if (local_env.fault_occurred) {
    xmlrpc_env_clean(&local_env);
    throw torrent::input_error("Unsupported XMLRPC dialect selected.");
  }

  xmlrpc_env_clean(&local_env);
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

} // namespace rpc

#endif
