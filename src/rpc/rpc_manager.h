#ifndef RTORRENT_RPC_MANAGER_H
#define RTORRENT_RPC_MANAGER_H

#include <cstdint>
#include <functional>
#include <torrent/common.h>

#include "rpc/command.h"
#include "rpc/command_map.h"
#include "rpc/exec_file.h"
#include "rpc/jsonrpc.h"
#include "rpc/xmlrpc.h"

namespace core {
class Download;
}

namespace rpc {

extern ExecFile   execFile;
extern CommandMap commands;

class rpc_error : public torrent::base_error {
public:
  rpc_error(int type, std::string msg) :
      m_type(type), m_msg(msg) {}
  virtual ~rpc_error() throw() = default;

  virtual int         type() const throw() { return m_type; }
  virtual const char* what() const throw() { return m_msg.c_str(); }

private:
  int         m_type;
  std::string m_msg;
};

class RpcManager {
public:
  using slot_download          = std::function<core::Download*(const char*)>;
  using slot_file              = std::function<torrent::File*(core::Download*, uint32_t)>;
  using slot_tracker           = std::function<torrent::tracker::Tracker(core::Download*, uint32_t)>;
  using slot_peer              = std::function<torrent::Peer*(core::Download*, const torrent::HashString&)>;
  using slot_response_callback = std::function<bool(const char*, uint32_t)>;

  enum RPCType { XML,
                 JSON };

  RpcManager()  = default;
  ~RpcManager() = default;

  bool           is_handlers_initialized() const { return m_handlers_initialized; }

  void           initialize_handlers();
  void           cleanup();

  int64_t        size_limit()                  { return m_xmlrpc.size_limit(); };
  void           set_size_limit(uint64_t size) { m_xmlrpc.set_size_limit(size); };

  int            dialect()                     { return m_xmlrpc.dialect(); }
  void           set_dialect(int dialect)      { m_xmlrpc.set_dialect(dialect); }

  bool           is_type_enabled(RPCType type) const;
  void           set_type_enabled(RPCType type, bool enabled);

  bool           process(RPCType type, const char* in_buffer, uint32_t length, slot_response_callback callback);

  void           insert_command(const char* name, const char* parm, const char* doc);

  slot_download& slot_find_download() { return m_slot_find_download; }
  slot_file&     slot_find_file()     { return m_slot_find_file; }
  slot_tracker&  slot_find_tracker()  { return m_slot_find_tracker; }
  slot_peer&     slot_find_peer()     { return m_slot_find_peer; }

  static void    object_to_target(const torrent::Object& obj, int callFlags, rpc::target_type* target, std::function<void()>* deleter);

private:
  XmlRpc        m_xmlrpc;
  JsonRpc       m_jsonrpc;

  bool          m_handlers_initialized{};
  bool          m_is_jsonrpc_enabled{true};
  bool          m_is_xmlrpc_enabled{true};

  slot_download m_slot_find_download;
  slot_file     m_slot_find_file;
  slot_tracker  m_slot_find_tracker;
  slot_peer     m_slot_find_peer;
};

extern RpcManager rpc;

} // namespace rpc

#endif
