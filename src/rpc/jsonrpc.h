#ifndef RTORRENT_RPC_JSONRPC_H
#define RTORRENT_RPC_JSONRPC_H

#include <functional>

#include <cstdint>

namespace rpc {

class JsonRpc {
public:
  using slot_write = std::function<bool(const char*, uint32_t)>;

  void initialize() {};
  void cleanup() {};

  bool process(const char* in_buffer, uint32_t length, slot_write callback);

  void insert_command(const char* name, const char* parm, const char* doc) {};
};

} // namespace rpc

#endif
