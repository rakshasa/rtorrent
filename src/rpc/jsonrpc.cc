#include "config.h"

#include "rpc/jsonrpc.h"

#include <cstdint>
#include <string>
#include <torrent/common.h>
#include <torrent/torrent.h>

#include "rpc/rpc_manager.h"
#include "rpc/command.h"
#include "rpc/command_map.h"
#include "rpc/nlohmann/json.h"
#include "rpc/parse_commands.h"
#include "torrent/exceptions.h"
#include "torrent/object.h"
#include "utils/functional.h"

namespace rpc {

// Taken from https://www.jsonrpc.org/specification
constexpr int JSONRPC_PARSE_ERROR            = -32700;
constexpr int JSONRPC_INVALID_REQUEST_ERROR  = -32600;
constexpr int JSONRPC_METHOD_NOT_FOUND_ERROR = -32601;
constexpr int JSONRPC_INVALID_PARAMS_ERROR   = -32602;
constexpr int JSONRPC_INTERNAL_ERROR         = -32000;

using json = nlohmann::json;

torrent::Object
json_to_object(const json& value) {
  switch (value.type()) {
  case json::value_t::number_unsigned:
  case json::value_t::number_integer:
    return torrent::Object(value.get<int64_t>());
  case json::value_t::boolean:
    return value.get<bool>() ? torrent::Object(int64_t(1)) : torrent::Object(int64_t(0));
  case json::value_t::string:
    return torrent::Object(value.get<std::string>());
  case json::value_t::array: {
    auto  array_raw = torrent::Object::create_list();
    auto& array     = array_raw.as_list();
    for (const auto& entry : value) {
      array.push_back(json_to_object(entry));
    }
    return array_raw;
  }
  case json::value_t::object: {
    auto  map_raw = torrent::Object::create_map();
    auto& map     = map_raw.as_map();
    for (const auto& entry : value.items()) {
      map[entry.key()] = json_to_object(entry.value());
    }
    return map_raw;
  }
  case json::value_t::number_float:
    // type_name() for floats returns 'number', which is accurate for JSON but misleading
    throw torrent::input_error("invalid parameters: unexpected data type float");
  default:
    throw torrent::input_error("invalid parameters: unexpected data type " + std::string(value.type_name()));
  }
}

json
object_to_json(const torrent::Object& object) noexcept {
  switch (object.type()) {
  case torrent::Object::TYPE_VALUE:
    return object.as_value();
  case torrent::Object::TYPE_STRING:
    return object.as_string();
  case torrent::Object::TYPE_LIST: {
    json result = json::array();
    for (const auto& obj : object.as_list())
      result.push_back(object_to_json(obj));
    return result;
  }
  case torrent::Object::TYPE_MAP: {
    json result = json::object();
    for (const auto& entry : object.as_map()) {
      result.emplace(entry.first, object_to_json(entry.second));
    }
    return result;
  }
  case torrent::Object::TYPE_DICT_KEY: {
    json result = json::array();
    result.push_back(object_to_json(object.as_dict_key()));

    const auto& dict_obj = object.as_dict_obj();
    if (dict_obj.is_list()) {
      for (const auto& element : dict_obj.as_list())
        result.push_back(object_to_json(element));
    } else {
      result.push_back(object_to_json(dict_obj));
    }

    return result;
  }
  default:
    return 0;
  }
}

json
jsonrpc_call_command(const std::string& method, const json& params) {
  if (params.type() == json::value_t::object) {
    // Named parameters is valid JSON-RPC, rtorrent just doesn't support it
    throw rpc_error(JSONRPC_INVALID_PARAMS_ERROR, "invalid parameter: procedure named parameter not supported");
  } else if (params.type() != json::value_t::array) {
    throw rpc_error(JSONRPC_INVALID_REQUEST_ERROR, "invalid request: params field must be an array");
  }

  CommandMap::iterator itr = commands.find(method.c_str());

  if (itr == commands.end()) {
    throw rpc_error(JSONRPC_METHOD_NOT_FOUND_ERROR, "method not found: " + method);
  }

  torrent::Object  params_object      = json_to_object(params);
  auto&            params_object_list = params_object.as_list();
  rpc::target_type target             = rpc::make_target();

  std::function<void()> deleter = []() {};
  utils::scope_guard    guard([&deleter]() { deleter(); });

  // Provide a blank target if none was provided
  if (params_object_list.empty())
    params_object_list.push_back("");

  if (!params_object_list.begin()->is_string())
    throw torrent::input_error("invalid parameters: target must be a string");

  RpcManager::object_to_target(params_object_list.begin()->as_string(), itr->second.m_flags, &target, &deleter);

  params_object_list.erase(params_object_list.begin());

  const auto& result = rpc::commands.call_command(itr, params_object, target);

  return object_to_json(result);
}

json
json_error(int code, const std::string& msg, json id) {
  return json{{"jsonrpc", "2.0"}, {"id", id}, {"error", {{"code", code}, {"message", msg}}}};
}

json
handle_request(const json& request) {
  json response = {{"jsonrpc", "2.0"}, {"id", nullptr}};

  try {
    if (!request["id"].is_number() && !request["id"].is_string() && !request["id"].is_null())
      return json_error(JSONRPC_INVALID_REQUEST_ERROR, "request id is invalid type " + std::string(request["id"].type_name()), response["id"]);
    response["id"] = request["id"];
    if (!request.contains("method") || !request["method"].is_string())
      return json_error(JSONRPC_INVALID_REQUEST_ERROR, "method string not present", response["id"]);
    if (request.contains("params"))
      response["result"] = jsonrpc_call_command(request["method"], request["params"]);
    else
      response["result"] = jsonrpc_call_command(request["method"], json::array({""}));
    return response;
  } catch (rpc_error& e) {
    return json_error(e.type(), e.what(), response["id"]);
  } catch (torrent::input_error& e) {
    return json_error(JSONRPC_INVALID_PARAMS_ERROR, e.what(), response["id"]);
  } catch (torrent::local_error& e) {
    return json_error(JSONRPC_INTERNAL_ERROR, e.what(), response["id"]);
  }
}

// Notifications are basically the same as requests, except we can
// just drop the message on the floor if there are any errors
void
handle_notification(const json& request) noexcept {
  if (!request.contains("method") || !request["method"].is_string())
    return;
  try {
    if (request.contains("params"))
      jsonrpc_call_command(request["method"], request["params"]);
    else
      jsonrpc_call_command(request["method"], json::array({""}));
  } catch (std::exception& e) {
  }
}

bool
JsonRpc::process(const char* in_buffer, uint32_t length, slot_write callback) {
  json response;
  json body;

  try {
    body = json::parse(in_buffer, in_buffer + length);
    switch (body.type()) {
    case json::value_t::object: {
      if (!body.contains("id")) {
        handle_notification(body);
        return callback("", 0);
      } else {
        response = handle_request(body);
      }
      break;
    }
    case json::value_t::array: {
      // Empty batch requests are invalid as per the spec
      if (body.empty()) {
        response = json_error(JSONRPC_INVALID_REQUEST_ERROR, "invalid request: empty batch", nullptr);
        break;
      }
      response = json::array();
      for (const auto& sub_body : body) {
        if (!sub_body.contains("id"))
          handle_notification(sub_body);
        else
          response.push_back(handle_request(sub_body));
      }
      // This indicates the batch was composed entirely of
      // notifications, in which case nothing is returned
      if (response.empty())
        return callback("", 0);
      break;
    }
    default:
      response = json_error(JSONRPC_PARSE_ERROR, "message type " + std::string(body.type_name()) + " unsupported", nullptr);
    }

    std::string response_str = response.dump();

    return callback(response_str.c_str(), response_str.size());

  } catch (json::parse_error& e) {
    auto err_str = json_error(JSONRPC_PARSE_ERROR, e.what(), nullptr).dump(-1, ' ', false, json::error_handler_t::replace);
    return callback(err_str.c_str(), err_str.size());
  } catch (json::type_error& e) {
    // Type errors may be caused by invalid UTF-8 strings in exception strings, hence the ::replace
    auto err_str = json_error(JSONRPC_PARSE_ERROR, e.what(), nullptr).dump(-1, ' ', false, json::error_handler_t::replace);
    return callback(err_str.c_str(), err_str.size());
  }
}

} // namespace rpc
