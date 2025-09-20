#include "config.h"

#include <cstring>

#include <torrent/exceptions.h>

#include "parse_commands.h"
#include "rpc/rpc_manager.h"

namespace rpc {

CommandMap commands;
RpcManager rpc;
ExecFile   execFile;

void
RpcManager::object_to_target(const torrent::Object& obj, int call_flags, rpc::target_type* target, std::function<void()>* deleter) {
  if (!obj.is_string())
    throw torrent::input_error("invalid parameters: target must be a string");

  std::string target_string = obj.as_string();
  bool        require_index = (call_flags & (CommandMap::flag_tracker_target | CommandMap::flag_file_target));

  if (target_string.size() == 0 && !require_index)
    return;

  // Length of SHA1 hash is 40
  if (target_string.size() < 40)
    throw torrent::input_error("invalid parameters: invalid target");

  char        type = 'd';
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

  core::Download* download = rpc.slot_find_download()(hash.c_str());

  if (download == nullptr)
    throw torrent::input_error("invalid parameters: info-hash not found");

  try {
    torrent::tracker::Tracker* tracker{};

    switch (type) {
    case 'd':
      *target = rpc::make_target(download);
      break;

    case 'f':
      *target = rpc::make_target(command_base::target_file,
                                 rpc.slot_find_file()(download, std::stoi(std::string(index))));

      break;

    case 't':
      tracker = new torrent::tracker::Tracker(rpc.slot_find_tracker()(download, std::stoi(std::string(index))));

      *target = rpc::make_target(command_base::target_tracker, tracker);
      *deleter = [tracker]() { delete tracker; };
      break;

    case 'p': {
      if (index.size() < 40) {
        throw torrent::input_error("invalid parameters: not a hash string.");
      }

      torrent::HashString hash;
      torrent::hash_string_from_hex_c_str(index.c_str(), hash);

      *target = rpc::make_target(command_base::target_peer,
                                 rpc.slot_find_peer()(download, hash));

      break;
    }
    default:
      throw torrent::input_error("invalid parameters: unexpected target type");
    }

  } catch (const std::logic_error&) {
    throw torrent::input_error("invalid parameters: invalid index");
  }
}

bool
RpcManager::process(RPCType type, const char* in_buffer, uint32_t length, slot_response_callback callback) {
  switch (type) {
  case RPCType::XML:
    // TODO: 'network.rpc.use_xmlrpc' should be a bool in RpcManager, not a command variable.
    if (m_xmlrpc.is_valid() && rpc::call_command_value("network.rpc.use_xmlrpc")) {
      return m_xmlrpc.process(in_buffer, length, callback);

    } else {
      const std::string response = "<?xml version=\"1.0\"?><methodResponse><fault><value><struct><member><name>faultCode</name><value><i8>-501</i8></value></member><member><name>faultString</name><value><string>XML-RPC not supported</string></value></member></struct></value></fault></methodResponse>";
      return callback(response.c_str(), response.size());
    }
    break;

  case RPCType::JSON:
    if (rpc::call_command_value("network.rpc.use_jsonrpc")) {
      return m_jsonrpc.process(in_buffer, length, callback);

    } else {
      const std::string response = "{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32601,\"message\":\"JSON-RPC not supported\"},\"id\":null}";
      return callback(response.c_str(), response.size());
    }
    break;

  default:
    throw torrent::input_error("invalid parameters: unknown RPC type");
  }
}

void
RpcManager::initialize_handlers() {
  if (m_handlers_initialized)
    throw torrent::internal_error("handlers already initialized");

  m_xmlrpc.initialize();
  m_jsonrpc.initialize();

  m_handlers_initialized = true;
}

void
RpcManager::cleanup() {
  m_handlers_initialized = false;

  m_xmlrpc.cleanup();
  m_jsonrpc.cleanup();
}

bool
RpcManager::is_type_enabled(RPCType type) const {
  switch (type) {
  case RPCType::XML:
    return m_is_xmlrpc_enabled;
  case RPCType::JSON:
    return m_is_jsonrpc_enabled;
  default:
    throw torrent::input_error("invalid parameters: unknown RPC type");
  }
}

void
RpcManager::set_type_enabled(RPCType type, bool enabled) {
  switch (type) {
  case RPCType::XML:
    m_is_xmlrpc_enabled = enabled;
    break;
  case RPCType::JSON:
    m_is_jsonrpc_enabled = enabled;
    break;
  default:
    throw torrent::input_error("invalid parameters: unknown RPC type");
  }
}

void
RpcManager::insert_command(const char* name, const char* parm, const char* doc) {
  m_xmlrpc.insert_command(name, parm, doc);
  m_jsonrpc.insert_command(name, parm, doc);
}

} // namespace rpc
