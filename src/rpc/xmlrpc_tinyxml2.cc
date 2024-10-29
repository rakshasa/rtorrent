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

#ifdef HAVE_XMLRPC_TINYXML2
#ifdef HAVE_XMLRPC_C
#error HAVE_XMLRPC_C and HAVE_XMLRPC_TINYXML2 cannot be used together. Please choose only one
#endif

#include <cctype>
#include <string>

#include <stdlib.h>

#include <rak/string_manip.h>
#include <torrent/object.h>
#include <torrent/exceptions.h>

#include "rpc/tinyxml2/tinyxml2.h"
#include "utils/base64.h"
#include "xmlrpc.h"
#include "parse_commands.h"

namespace rpc {

// Taken from xmlrpc-c
const int XMLRPC_INTERNAL_ERROR = -500;
const int XMLRPC_TYPE_ERROR = -501;
const int XMLRPC_INDEX_ERROR = -502;
const int XMLRPC_PARSE_ERROR = -503;
const int XMLRPC_NETWORK_ERROR = -504;
const int XMLRPC_TIMEOUT_ERROR = -505;
const int XMLRPC_NO_SUCH_METHOD_ERROR = -506;
const int XMLRPC_REQUEST_REFUSED_ERROR = -507;
const int XMLRPC_INTROSPECTION_DISABLED_ERROR = -508;
const int XMLRPC_LIMIT_EXCEEDED_ERROR = -509;
const int XMLRPC_INVALID_UTF8_ERROR = -510;

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

const tinyxml2::XMLElement*
element_access(const tinyxml2::XMLElement* elem, std::string element_names) {
  // Helper function to check each step of a element access, in lieu of XPath
  const tinyxml2::XMLElement* result = elem;
  size_t pos = 0;
  do {
    auto previous_pos = pos;
    pos = element_names.find(',', pos + 1);
    auto item = element_names.substr(previous_pos, pos - previous_pos);
    result = result->FirstChildElement(item.c_str());
    if (result == nullptr) {
      throw xmlrpc_error(XMLRPC_PARSE_ERROR, "could not find expected element " + item);
    }
  } while (pos != std::string::npos);
  return result;
}

torrent::Object
xml_value_to_object(const tinyxml2::XMLNode* elem) {
  if (elem == nullptr) {
    throw xmlrpc_error(XMLRPC_INTERNAL_ERROR, "received null element to convert");
  }
  if (std::strncmp(elem->Value(), "value", 5) != 0) {
    throw xmlrpc_error(XMLRPC_INTERNAL_ERROR, "received non-value element to convert");
  }
  auto value_element = elem->FirstChild();
  auto value_element_type = std::string(value_element->Value());
  if (value_element_type == "string") {
    auto value_element_child = value_element->FirstChild();
    if (value_element_child == nullptr) {
      return torrent::Object("");
    }
    return torrent::Object(value_element_child->ToText()->Value());
  } else if (value_element_type == "i4" || value_element_type == "i8" || value_element_type == "int") {
    char* pos;
    auto str = value_element->FirstChild()->ToText()->Value();
    auto result = std::strtoll(str, &pos, 10);
    if (pos == str || *pos != '\0')
      throw xmlrpc_error(XMLRPC_TYPE_ERROR, "unable to parse integer value");
    return torrent::Object(result);
  } else if (value_element_type == "boolean") {
    auto boolean_text = std::string(value_element->FirstChild()->ToText()->Value());
    if (boolean_text == "1") {
      return torrent::Object((int64_t)1);
    } else if (boolean_text == "0") {
      return torrent::Object((int64_t)0);
    }
    throw xmlrpc_error(XMLRPC_TYPE_ERROR, "unknown boolean value: " + boolean_text);
  } else if (value_element_type == "array") {
    auto array = torrent::Object::create_list();
    auto data_element = element_access(value_element->ToElement(), "data");
    for (auto child = data_element->FirstChildElement("value"); child; child = child->NextSiblingElement("value")) {
      array.as_list().push_back(xml_value_to_object(child));
    }
    return array;
  } else if (value_element_type == "struct") {
    auto map = torrent::Object::create_map();
    for (auto child = value_element->FirstChildElement("member"); child; child = child->NextSiblingElement("member")) {
      auto key = child->FirstChildElement("name")->GetText();
      map.as_map()[key] = xml_value_to_object(element_access(child, "value"));
    }
    return map;
  } else if (value_element_type == "base64") {
    auto value_element_child = value_element->FirstChild();
    if (value_element_child == nullptr) {
      return torrent::Object("");
    }
    auto base64string = std::string(value_element_child->ToText()->Value());
    base64string.erase(std::remove_if(base64string.begin(), base64string.end(),
                                      [](char c) { return c == '\n' || c == '\r'; }),
                       base64string.end());
    return torrent::Object(utils::base64decode(base64string));
  } else {
    throw xmlrpc_error(XMLRPC_INTERNAL_ERROR, "received unsupported value type: " + value_element_type);
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
    if (obj.as_value() > ((torrent::Object::value_type)2 << 30) || obj.as_value() < -((torrent::Object::value_type)2 << 30)) {
      printer->OpenElement("i8", true);
    } else {
      printer->OpenElement("i4", true);
    }
    printer->PushText(std::to_string(obj.as_value()).c_str());
    printer->CloseElement(true);
    break;
  case torrent::Object::TYPE_LIST:
    printer->OpenElement("array", true);
    for (const auto& itr : obj.as_list()) {
      printer->OpenElement("value", true);
      print_object_xml(itr, printer);
      printer->CloseElement(true);
    }
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

    printer->OpenElement("value", true);
    print_object_xml(obj.as_dict_key(), printer);
    printer->CloseElement(true);

    if (obj.as_dict_obj().is_list()) {
      for (auto itr : obj.as_dict_obj().as_list()) {
        printer->OpenElement("value", true);
        print_object_xml(obj.as_dict_key(), printer);
        printer->CloseElement(true);
      }
    } else {
      printer->OpenElement("value", true);
      print_object_xml(obj.as_dict_obj(), printer);
      printer->CloseElement(true);
    }
    printer->CloseElement(true);
    break;
  default:
    printer->OpenElement("i4", true);
    printer->PushText(0);
    printer->CloseElement(true);
  }
}

void
object_to_target(const torrent::Object& obj, int callFlags, rpc::target_type* target) {
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
            throw xmlrpc_error(XMLRPC_TYPE_ERROR, "Not a hash string.");
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

torrent::Object execute_command(std::string method_name, const tinyxml2::XMLElement* params_element) {
  if (params_element == nullptr) {
    throw xmlrpc_error(XMLRPC_INTERNAL_ERROR, "invalid parameters: null");
  }
  CommandMap::iterator cmd_itr = commands.find(method_name.c_str());
  if (cmd_itr == commands.end() || !(cmd_itr->second.m_flags & CommandMap::flag_public_xmlrpc)) {
    throw xmlrpc_error(XMLRPC_NO_SUCH_METHOD_ERROR, "Method '" + std::string(method_name) + "' not defined");
  }
  torrent::Object params_raw = torrent::Object::create_list();
  torrent::Object::list_type& params = params_raw.as_list();
  if (params_element != nullptr) {
    for (auto child = params_element->FirstChildElement("param"); child; child = child->NextSiblingElement("param")) {
      params.push_back(xml_value_to_object(child->FirstChildElement("value")));
    }
  }
  rpc::target_type target = rpc::make_target();
  if (params.size() == 0 && (cmd_itr->second.m_flags & (CommandMap::flag_file_target | CommandMap::flag_tracker_target))) {
    throw xmlrpc_error(XMLRPC_TYPE_ERROR, "invalid parameters: too few");
  }
  if (params.size() > 0) {
    object_to_target(params.front(), cmd_itr->second.m_flags, &target);
    params.erase(params.begin());
  }
  return rpc::commands.call_command(cmd_itr, params_raw, target);
}

void
process_document(const tinyxml2::XMLDocument* doc, tinyxml2::XMLPrinter* printer) {
  if (doc->Error()) {
    throw xmlrpc_error(XMLRPC_PARSE_ERROR, doc->ErrorStr());
  }
  if (doc->FirstChildElement("methodCall") == nullptr) {
    throw xmlrpc_error(XMLRPC_PARSE_ERROR, "methodCall element not found");
  }
  auto method_name = element_access(doc->FirstChildElement("methodCall"), "methodName")->GetText();
  torrent::Object result;

  // Add a shim here for system.multicall to allow better code reuse, and
  // because system.multicall is one of the few methods that doesn't take a target
  if (method_name == std::string("system.multicall")) {
    result = torrent::Object::create_list();
    torrent::Object::list_type& result_list = result.as_list();
    auto value_elements = element_access(doc->RootElement(), "params,param,value,array,data");
    for (auto child = value_elements->FirstChildElement("value"); child; child = child->NextSiblingElement("value")) {
      auto sub_method_name = element_access(child, "struct,member,value,string")->GetText();
      auto sub_params = element_access(child, "struct,member")->NextSiblingElement("member")->FirstChildElement("value");
      try {
        auto sub_result = torrent::Object::create_list();
        sub_result.as_list().push_back(execute_command(sub_method_name, sub_params));
        result_list.push_back(sub_result);
      } catch (xmlrpc_error& e) {
        auto fault = torrent::Object::create_map();
        fault.as_map()["faultString"] = e.what();
        fault.as_map()["faultCode"] = e.type();
        result_list.push_back(fault);
      } catch (torrent::local_error& e) {
        auto fault = torrent::Object::create_map();
        fault.as_map()["faultString"] = e.what();
        fault.as_map()["faultCode"] = XMLRPC_INTERNAL_ERROR;
        result_list.push_back(fault);
      }
    }
  } else {
    result = execute_command(method_name, doc->FirstChildElement("methodCall")->FirstChildElement("params"));
  }

  printer->PushHeader(false, true);
  printer->OpenElement("methodReponse", true);
  printer->OpenElement("params", true);

  printer->OpenElement("param", true);
  printer->OpenElement("value", true);
  print_object_xml(result, printer);
  printer->CloseElement(true);
  printer->CloseElement(true);

  printer->CloseElement(true);
  printer->CloseElement(true);
  tinyxml2::XMLDocument resultDoc;
}


void
print_xmlrpc_fault(int faultCode, std::string faultString, tinyxml2::XMLPrinter* printer) {
  printer->PushHeader(false, true);

  printer->OpenElement("methodReponse", true);
  printer->OpenElement("fault", true);
  printer->OpenElement("struct", true);

  printer->OpenElement("member", true);
  printer->OpenElement("name", true);
  printer->PushText("faultCode");
  printer->CloseElement(true);
  printer->OpenElement("value", true);
  printer->OpenElement("int", true);
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
}

bool
XmlRpc::process(const char* inBuffer, uint32_t length, slot_write slotWrite) {
  tinyxml2::XMLDocument doc;
  doc.Parse(inBuffer, length);
  try {
    // This printer can't be reused in the 'catch' because while the
    // buffer can be cleared, the internal stack of opened elements
    // remains.
    tinyxml2::XMLPrinter printer(nullptr, true, 0);
    process_document(&doc, &printer);
    return slotWrite(printer.CStr(), printer.CStrSize()-1);
  } catch (xmlrpc_error& e) {
    tinyxml2::XMLPrinter printer(nullptr, true, 0);
    print_xmlrpc_fault(e.type(), e.what(), &printer);
    return slotWrite(printer.CStr(), printer.CStrSize()-1);
  } catch (torrent::local_error& e) {
    tinyxml2::XMLPrinter printer(nullptr, true, 0);
    print_xmlrpc_fault(XMLRPC_INTERNAL_ERROR, e.what(), &printer);
    return slotWrite(printer.CStr(), printer.CStrSize()-1);
  }
}

void XmlRpc::initialize() { m_isValid = true; }
void XmlRpc::cleanup() {}

void XmlRpc::insert_command(const char*, const char*, const char*) {}
void XmlRpc::set_dialect(int) {}

int64_t XmlRpc::size_limit() { return std::numeric_limits<int64_t>::max(); }
void    XmlRpc::set_size_limit(uint64_t) {}

bool    XmlRpc::is_valid() const { return m_isValid; }

}

#endif
