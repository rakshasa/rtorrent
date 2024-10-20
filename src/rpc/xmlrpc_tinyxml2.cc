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
#ifndef HAVE_XMLRPC_C

#include <cctype>
#include <string>

#include <stdlib.h>

#include <rak/string_manip.h>
#include <torrent/object.h>
#include <torrent/exceptions.h>

#include "vendor/tinyxml2/tinyxml2.h"
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
element_access(const tinyxml2::XMLElement* elem, std::string elemNames) {
  // Helper function to check each step of a element access, in lieu of XPath
  char token = ',';
  std::string stack("");
  const tinyxml2::XMLElement* result = elem;
  size_t start = 0;
  size_t end = 0;
  do {
    end = elemNames.find(token, start);
    auto item = elemNames.substr(start, end - start);
    stack.append(item);
    result = result->FirstChildElement(item.c_str());
    if (result == nullptr) {
      throw xmlrpc_error(XMLRPC_PARSE_ERROR, "could not find expected element " + stack);
    }
    stack.append("->");
    start = end + 1;
  } while (end != std::string::npos);
  return result;
}

torrent::Object
xml_value_to_object(const tinyxml2::XMLNode* elem) {
  if (elem == nullptr) {
    throw xmlrpc_error(XMLRPC_INTERNAL_ERROR, "received null element to convert");
  }
  if (std::string("value") != elem->Value()) {
    throw xmlrpc_error(XMLRPC_INTERNAL_ERROR, "received non-value element to convert");
  }
  auto valueElem = elem->FirstChild();
  auto elemType = std::string(valueElem->Value());
  if (elemType == "string") {
    auto elemChild = valueElem->FirstChild();
    if (elemChild == nullptr) {
      return torrent::Object("");
    }
    return torrent::Object(elemChild->ToText()->Value());
  } else if (elemType == "i4" or elemType == "i8" or elemType == "int") {
    auto elemValue = std::string(valueElem->FirstChild()->ToText()->Value());
    return torrent::Object(std::stol(elemValue));
  } else if (elemType == "boolean") {
    auto boolText = std::string(valueElem->FirstChild()->ToText()->Value());
    if (boolText == "1") {
      return torrent::Object((int64_t)1);
    } else if (boolText == "0") {
      return torrent::Object((int64_t)0);
    }
  } else if (elemType == "array") {
    auto array = torrent::Object::create_list();
    auto dataElem = element_access(valueElem->ToElement(), "data");
    for (auto arrayValueElem = dataElem->FirstChildElement("value"); arrayValueElem; arrayValueElem = arrayValueElem->NextSiblingElement("value")) {
      array.as_list().push_back(xml_value_to_object(arrayValueElem));
    }
    return array;
  } else if (elemType == "struct") {
    auto map = torrent::Object::create_map();
    for (auto memberElem = valueElem->FirstChildElement("member"); memberElem; memberElem = memberElem->NextSiblingElement("member")) {
      auto key = memberElem->FirstChildElement("name")->GetText();
      auto valElem = xml_value_to_object(element_access(memberElem, "value"));
      map.as_map()[key] = valElem;
    }
    return map;
  } else if (elemType == "base64") {
    auto elemChild = valueElem->FirstChild();
    if (elemChild == nullptr) {
      return torrent::Object("");
    }
    auto base64string = std::string(elemChild->ToText()->Value());
    base64string.erase(std::remove_if(base64string.begin(), base64string.end(),
                                      [](char c) { return c == '\n' || c == '\r'; }),
                       base64string.end());
    return torrent::Object(utils::base64decode(base64string));
    return torrent::Object("");
  } else {
    throw xmlrpc_error(XMLRPC_INTERNAL_ERROR, "received unsupported value type: " + elemType);
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
  std::string targetString = obj.as_string();
  bool requireIndex = (callFlags & (CommandMap::flag_tracker_target | CommandMap::flag_file_target));
  if (targetString.size() == 0 && !requireIndex) {
    return;
  }

  // Length of SHA1 hash is 40
  if (targetString.size() < 40) {
    throw torrent::input_error("invalid parameters: invalid target");
  }

  char type = 'd';
  std::string hash;
  std::string index;
  const auto& delimPos = targetString.find_first_of(':', 40);
  if (delimPos == targetString.npos ||
      delimPos + 2 >= targetString.size()) {
	if (requireIndex) {
      throw torrent::input_error("invalid parameters: no index");
    }
    hash = targetString;
  } else {
    hash  = targetString.substr(0, delimPos);
    type  = targetString[delimPos + 1];
    index = targetString.substr(delimPos + 2);
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

torrent::Object execute_command(std::string methodName, const tinyxml2::XMLElement* paramsElem) {
  if (paramsElem == nullptr) {
    throw xmlrpc_error(XMLRPC_INTERNAL_ERROR, "invalid parameters: null");
  }
  CommandMap::iterator cmdItr = commands.find(methodName.c_str());
  if (cmdItr == commands.end() || !(cmdItr->second.m_flags & CommandMap::flag_public_xmlrpc)) {
    throw xmlrpc_error(XMLRPC_NO_SUCH_METHOD_ERROR, "Method '" + std::string(methodName) + "' not defined");
  }
  torrent::Object paramsRaw = torrent::Object::create_list();
  torrent::Object::list_type& params = paramsRaw.as_list();
  if (paramsElem != nullptr) {
    for (auto paramElem = paramsElem->FirstChildElement("param"); paramElem; paramElem = paramElem->NextSiblingElement("param")) {
      params.push_back(xml_value_to_object(paramElem->FirstChildElement("value")));
    }
  }
  rpc::target_type target = rpc::make_target();
  if (params.size() == 0 && (cmdItr->second.m_flags & (CommandMap::flag_file_target | CommandMap::flag_tracker_target))) {
    throw xmlrpc_error(XMLRPC_INTERNAL_ERROR, "invalid parameters: too few");
  }
  if (params.size() > 0) {
    object_to_target(params.front(), cmdItr->second.m_flags, &target);
    params.erase(params.begin());
  }
  return rpc::commands.call_command(cmdItr, paramsRaw, target);
}

void
process_document(const tinyxml2::XMLDocument* doc, tinyxml2::XMLPrinter* printer) {
  if (doc->Error()) {
    throw xmlrpc_error(XMLRPC_PARSE_ERROR, doc->ErrorStr());
  }
  if (doc->FirstChildElement("methodCall") == nullptr) {
    throw xmlrpc_error(XMLRPC_PARSE_ERROR, "methodCall element not found");
  }
  auto methodName = element_access(doc->FirstChildElement("methodCall"), "methodName")->GetText();
  torrent::Object result;

  // Add a shim here for system.multicall to allow better code reuse, and
  // because system.multicall is one of the few methods that doesn't take a target
  if (methodName == std::string("system.multicall")) {
    result = torrent::Object::create_list();
    torrent::Object::list_type& resultList = result.as_list();
    auto valueElems = element_access(doc->RootElement(), "params,param,value,array,data");
    for (auto structElem = valueElems->FirstChildElement("value"); structElem; structElem = structElem->NextSiblingElement("value")) {
      auto subMethodName = element_access(structElem, "struct,member,value,string")->GetText();
      auto subParams = element_access(structElem, "struct,member")->NextSiblingElement("member")->FirstChildElement("value");
      try {
        auto subResult = torrent::Object::create_list();
        subResult.as_list().push_back(execute_command(subMethodName, subParams));
        resultList.push_back(subResult);
      } catch (xmlrpc_error& e) {
        auto fault = torrent::Object::create_map();
        fault.as_map()["faultString"] = e.what();
        fault.as_map()["faultCode"] = e.type();
        resultList.push_back(fault);
      } catch (torrent::local_error& e) {
        auto fault = torrent::Object::create_map();
        fault.as_map()["faultString"] = e.what();
        fault.as_map()["faultCode"] = XMLRPC_INTERNAL_ERROR;
        resultList.push_back(fault);
      }
    }
  } else {
    result = execute_command(methodName, doc->FirstChildElement("methodCall")->FirstChildElement("params"));
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
    // buffer can be cleared, the internal stack of opened tags
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

#endif // ifndef HAVE_XMLRPC_C
#endif // ifdef HAVE_XMLRPC_TINYXML2
