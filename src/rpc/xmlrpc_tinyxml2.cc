#include "config.h"

#ifdef HAVE_XMLRPC_TINYXML2
#ifdef HAVE_XMLRPC_C
#error HAVE_XMLRPC_C and HAVE_XMLRPC_TINYXML2 cannot be used together. Please choose only one
#endif

#include <cctype>
#include <initializer_list>
#include <string>

#include <stdlib.h>

#include <rak/string_manip.h>
#include <torrent/exceptions.h>
#include <torrent/object.h>

#include "parse_commands.h"
#include "rpc/tinyxml2/tinyxml2.h"
#include "rpc/rpc_manager.h"
#include "utils/base64.h"
#include "xmlrpc.h"

namespace rpc {

// Taken from xmlrpc-c
const int XMLRPC_INTERNAL_ERROR               = -500;
const int XMLRPC_TYPE_ERROR                   = -501;
// const int XMLRPC_INDEX_ERROR                  = -502;
const int XMLRPC_PARSE_ERROR                  = -503;
// const int XMLRPC_NETWORK_ERROR                = -504;
// const int XMLRPC_TIMEOUT_ERROR                = -505;
const int XMLRPC_NO_SUCH_METHOD_ERROR         = -506;
// const int XMLRPC_REQUEST_REFUSED_ERROR        = -507;
// const int XMLRPC_INTROSPECTION_DISABLED_ERROR = -508;
const int XMLRPC_LIMIT_EXCEEDED_ERROR         = -509;
// const int XMLRPC_INVALID_UTF8_ERROR           = -510;

const tinyxml2::XMLElement*
element_access(const tinyxml2::XMLElement* elem, std::initializer_list<std::string> names) {
  // Helper function to check each step of a element access, in lieu of XPath
  const tinyxml2::XMLElement* result = elem;
  for (auto itr : names) {
    result = result->FirstChildElement(itr.c_str());
    if (result == nullptr) {
      throw rpc_error(XMLRPC_PARSE_ERROR, "could not find expected element " + itr);
    }
  }
  return result;
}

long long
element_to_int(const tinyxml2::XMLNode* elem) {
  char* pos;
  if (elem->FirstChild() == nullptr) {
    throw rpc_error(XMLRPC_TYPE_ERROR, "unable to parse empty integer");
  }
  auto str    = elem->FirstChild()->ToText()->Value();
  auto result = std::strtoll(str, &pos, 10);
  if (pos == str || *pos != '\0')
    throw rpc_error(XMLRPC_TYPE_ERROR, "unable to parse integer value");
  return result;
}

torrent::Object
xml_value_to_object(const tinyxml2::XMLNode* elem) {
  if (elem == nullptr) {
    throw rpc_error(XMLRPC_INTERNAL_ERROR, "received null element to convert");
  }
  if (std::strncmp(elem->Value(), "value", sizeof("value")) != 0) {
    throw rpc_error(XMLRPC_INTERNAL_ERROR, "received non-value element to convert");
  }
  auto root_element = elem->FirstChild();
  auto root_type    = root_element->Value();
  if (std::strncmp(root_type, "string", sizeof("string")) == 0) {
    auto child_element = root_element->FirstChild();
    if (child_element == nullptr) {
      return torrent::Object("");
    }
    return torrent::Object(child_element->ToText()->Value());
  } else if (std::strncmp(root_type, "int", sizeof("int")) == 0 ||
             std::strncmp(root_type, "i4", sizeof("i4")) == 0 ||
             std::strncmp(root_type, "i8", sizeof("i8")) == 0) {
    return torrent::Object(element_to_int(root_element));
  } else if (std::strncmp(root_type, "boolean", sizeof("boolean")) == 0) {
    auto boolean_text = std::string(root_element->FirstChild()->ToText()->Value());
    if (boolean_text == "1") {
      return torrent::Object((int64_t)1);
    } else if (boolean_text == "0") {
      return torrent::Object((int64_t)0);
    }
    throw rpc_error(XMLRPC_TYPE_ERROR, "unknown boolean value: " + boolean_text);
  } else if (std::strncmp(root_type, "array", sizeof("array")) == 0) {
    auto  array_raw    = torrent::Object::create_list();
    auto& array        = array_raw.as_list();
    auto  data_element = root_element->ToElement()->FirstChildElement("data");
    if (data_element == nullptr)
      throw rpc_error(XMLRPC_PARSE_ERROR, "could not find expected data element in array");
    for (auto child = data_element->FirstChildElement("value"); child; child = child->NextSiblingElement("value")) {
      array.push_back(xml_value_to_object(child));
    }
    return array_raw;
  } else if (std::strncmp(root_type, "struct", sizeof("struct")) == 0) {
    auto  map_raw = torrent::Object::create_map();
    auto& map     = map_raw.as_map();
    for (auto child = root_element->FirstChildElement("member"); child; child = child->NextSiblingElement("member")) {
      auto key = child->FirstChildElement("name")->GetText();
      map[key] = std::move(xml_value_to_object(child->FirstChildElement("value")));
    }
    return map_raw;
  } else if (std::strncmp(root_type, "base64", sizeof("base64")) == 0) {
    auto child_element = root_element->FirstChild();
    if (child_element == nullptr) {
      return torrent::Object("");
    }
    return torrent::Object(utils::decode_base64(utils::remove_newlines(child_element->ToText()->Value())));
  } else {
    throw rpc_error(XMLRPC_INTERNAL_ERROR, "received unsupported value type: " + std::string(root_type));
  }
  return torrent::Object();
}

void
print_object_xml(const torrent::Object& obj, tinyxml2::XMLPrinter* printer) {
  switch (obj.type()) {
  case torrent::Object::TYPE_STRING:
    printer->OpenElement("string", true);
    printer->PushText(obj.as_string().c_str());
    printer->CloseElement(true);
    break;
  case torrent::Object::TYPE_VALUE:
    printer->OpenElement("i8", true);
    printer->PushText(std::to_string(obj.as_value()).c_str());
    printer->CloseElement(true);
    break;
  case torrent::Object::TYPE_LIST:
    printer->OpenElement("array", true);
    printer->OpenElement("data", true);
    for (const auto& itr : obj.as_list()) {
      printer->OpenElement("value", true);
      print_object_xml(itr, printer);
      printer->CloseElement(true);
    }
    printer->CloseElement(true);
    printer->CloseElement(true);
    break;
  case torrent::Object::TYPE_MAP:
    printer->OpenElement("struct", true);
    for (const auto& itr : obj.as_map()) {
      printer->OpenElement("member", true);
      printer->OpenElement("name", true);
      printer->PushText(itr.first.c_str());
      printer->CloseElement(true);
      printer->OpenElement("value", true);
      print_object_xml(itr.second, printer);
      printer->CloseElement(true);
      printer->CloseElement(true);
    }
    printer->CloseElement(true);
    break;
  case torrent::Object::TYPE_DICT_KEY:
    printer->OpenElement("array", true);
    printer->OpenElement("data", true);
    printer->OpenElement("value", true);
    print_object_xml(obj.as_dict_key(), printer);
    printer->CloseElement(true);

    if (obj.as_dict_obj().is_list()) {
      for (auto itr : obj.as_dict_obj().as_list()) {
        printer->OpenElement("value", true);
        print_object_xml(itr, printer);
        printer->CloseElement(true);
      }
    } else {
      printer->OpenElement("value", true);
      print_object_xml(obj.as_dict_obj(), printer);
      printer->CloseElement(true);
    }
    printer->CloseElement(true);
    printer->CloseElement(true);
    break;
  default:
    printer->OpenElement("i8", true);
    printer->PushText(0);
    printer->CloseElement(true);
  }
}

torrent::Object
execute_command(std::string method_name, const tinyxml2::XMLElement* params_element) {
  CommandMap::iterator cmd_itr = commands.find(method_name.c_str());

  if (cmd_itr == commands.end() || !(cmd_itr->second.m_flags & CommandMap::flag_public_rpc)) {
    throw rpc_error(XMLRPC_NO_SUCH_METHOD_ERROR, "method '" + method_name + "' not defined");
  }

  torrent::Object             params_raw = torrent::Object::create_list();
  torrent::Object::list_type& params     = params_raw.as_list();
  rpc::target_type            target     = rpc::make_target();

  if (params_element != nullptr) {
    if (std::strncmp(params_element->Name(), "params", sizeof("params")) == 0) {
      // Parse out the target if available
      const auto* child = params_element->FirstChildElement("param");

      if (child != nullptr) {
        std::function<void()> deleter = []() {};

        RpcManager::object_to_target(xml_value_to_object(child->FirstChildElement("value")), cmd_itr->second.m_flags, &target, &deleter);
        child = child->NextSiblingElement("param");

        // Parse out any other params
        while (child != nullptr) {
          params.push_back(xml_value_to_object(child->FirstChildElement("value")));
          child = child->NextSiblingElement("param");
        }
      }

    } else if (params_element->FirstChildElement("data") != nullptr) {
      // If it's not a <params>, it's probably a <array> passed in via system.multicall
      const auto* child = params_element->FirstChildElement("data")->FirstChildElement("value");

      if (child != nullptr) {
        std::function<void()> deleter = []() {};

        RpcManager::object_to_target(xml_value_to_object(child), cmd_itr->second.m_flags, &target, &deleter);
        child = child->NextSiblingElement("value");

        while (child != nullptr) {
          params.push_back(xml_value_to_object(child));
          child = child->NextSiblingElement("value");
        }
      }
    }
  }

  if (params.empty() && (cmd_itr->second.m_flags & (CommandMap::flag_file_target | CommandMap::flag_tracker_target))) {
    throw rpc_error(XMLRPC_TYPE_ERROR, "invalid parameters: too few");
  }

  return rpc::commands.call_command(cmd_itr, params_raw, target);
}

void
process_document(const tinyxml2::XMLDocument* doc, tinyxml2::XMLPrinter* printer) {
  if (doc->Error())
    throw rpc_error(XMLRPC_PARSE_ERROR, doc->ErrorStr());
  if (doc->FirstChildElement("methodCall") == nullptr)
    throw rpc_error(XMLRPC_PARSE_ERROR, "methodCall element not found");
  if (doc->FirstChildElement("methodCall")->FirstChildElement("methodName") == nullptr)
    throw rpc_error(XMLRPC_PARSE_ERROR, "methodName element not found");
  auto            method_name = doc->FirstChildElement("methodCall")->FirstChildElement("methodName")->GetText();
  torrent::Object result;

  // Add a shim here for system.multicall to allow better code reuse, and
  // because system.multicall is one of the few methods that doesn't take a target
  if (method_name == std::string("system.multicall")) {
    result                = torrent::Object::create_list();
    auto& result_list     = result.as_list();
    auto  parent_elements = element_access(doc->RootElement(), {"params", "param", "value", "array", "data"});
    for (auto child = parent_elements->FirstChildElement("value"); child; child = child->NextSiblingElement("value")) {
      auto sub_method_name = element_access(child, {"struct", "member", "value", "string"})->GetText();
      // If sub_params ends up a nullptr at the end of this if-chian,
      // execute_command will turn it into an empty list
      auto sub_params = element_access(child, {"struct", "member"});
      if (sub_params != nullptr)
        sub_params = sub_params->NextSiblingElement("member");
      if (sub_params != nullptr)
        sub_params = sub_params->FirstChildElement("value");
      if (sub_params != nullptr)
        sub_params = sub_params->FirstChildElement("array");
      try {
        auto sub_result = torrent::Object::create_list();
        sub_result.as_list().push_back(execute_command(sub_method_name, sub_params));
        result_list.push_back(sub_result);
      } catch (rpc_error& e) {
        auto fault                    = torrent::Object::create_map();
        fault.as_map()["faultString"] = e.what();
        fault.as_map()["faultCode"]   = e.type();
        result_list.push_back(fault);
      } catch (torrent::local_error& e) {
        auto fault                    = torrent::Object::create_map();
        fault.as_map()["faultString"] = e.what();
        fault.as_map()["faultCode"]   = XMLRPC_INTERNAL_ERROR;
        result_list.push_back(fault);
      }
    }
  } else {
    result = execute_command(method_name, doc->FirstChildElement("methodCall")->FirstChildElement("params"));
  }

  printer->PushHeader(false, true);
  printer->OpenElement("methodResponse", true);
  printer->OpenElement("params", true);

  printer->OpenElement("param", true);
  printer->OpenElement("value", true);
  print_object_xml(result, printer);
  printer->CloseElement(true);
  printer->CloseElement(true);

  printer->CloseElement(true);
  printer->CloseElement(true);
}

void
print_xmlrpc_fault(int faultCode, std::string faultString, tinyxml2::XMLPrinter* printer) {
  printer->PushHeader(false, true);

  printer->OpenElement("methodResponse", true);
  printer->OpenElement("fault", true);
  printer->OpenElement("value", true);
  printer->OpenElement("struct", true);

  printer->OpenElement("member", true);
  printer->OpenElement("name", true);
  printer->PushText("faultCode");
  printer->CloseElement(true);
  printer->OpenElement("value", true);
  printer->OpenElement("i8", true);
  printer->PushText(faultCode);
  printer->CloseElement(true);
  printer->CloseElement(true);
  printer->CloseElement(true);

  printer->OpenElement("member", true);
  printer->OpenElement("name", true);
  printer->PushText("faultString");
  printer->CloseElement(true);
  printer->OpenElement("value", true);
  printer->OpenElement("string", true);
  printer->PushText(faultString.c_str());
  printer->CloseElement(true);
  printer->CloseElement(true);
  printer->CloseElement(true);

  printer->CloseElement(true);
  printer->CloseElement(true);
  printer->CloseElement(true);
  printer->CloseElement(true);
}

bool
XmlRpc::process(const char* inBuffer, uint32_t length, slot_write slotWrite) {
  if (length > m_sizeLimit) {
    tinyxml2::XMLPrinter printer(nullptr, true, 0);
    print_xmlrpc_fault(XMLRPC_LIMIT_EXCEEDED_ERROR, "Content size exceeds maximum XML-RPC limit", &printer);
    return slotWrite(printer.CStr(), printer.CStrSize() - 1);
  }
  tinyxml2::XMLDocument doc;
  doc.Parse(inBuffer, length);
  try {
    // This printer can't be reused in the 'catch' because while the
    // buffer can be cleared, the internal stack of opened elements
    // remains.
    tinyxml2::XMLPrinter printer(nullptr, true, 0);
    process_document(&doc, &printer);
    return slotWrite(printer.CStr(), printer.CStrSize() - 1);
  } catch (rpc_error& e) {
    tinyxml2::XMLPrinter printer(nullptr, true, 0);
    print_xmlrpc_fault(e.type(), e.what(), &printer);
    return slotWrite(printer.CStr(), printer.CStrSize() - 1);
  } catch (torrent::local_error& e) {
    tinyxml2::XMLPrinter printer(nullptr, true, 0);
    print_xmlrpc_fault(XMLRPC_INTERNAL_ERROR, e.what(), &printer);
    return slotWrite(printer.CStr(), printer.CStrSize() - 1);
  }
}

void
XmlRpc::initialize() { m_isValid = true; }
void
XmlRpc::cleanup() {}

void
XmlRpc::insert_command(const char*, const char*, const char*) {}
void
XmlRpc::set_dialect(int) {}

int64_t
XmlRpc::size_limit() { return static_cast<int64_t>(m_sizeLimit); }
void
XmlRpc::set_size_limit(uint64_t size) { m_sizeLimit = size; }

bool
XmlRpc::is_valid() const { return m_isValid; }

} // namespace rpc

#endif
