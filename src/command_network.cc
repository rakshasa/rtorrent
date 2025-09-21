#include "config.h"

#include <functional>
#include <cstdio>
#include <unistd.h>
#include <rak/address_info.h>
#include <rak/path.h>
#include <torrent/torrent.h>
#include <torrent/rate.h>
#include <torrent/connection_manager.h>
#include <torrent/data/file_manager.h>
#include <torrent/download/resource_manager.h>
#include <torrent/net/http_stack.h>
#include <torrent/net/network_config.h>
#include <torrent/net/socket_address.h>
#include <torrent/tracker/tracker.h>
#include <torrent/utils/log.h>
#include <torrent/utils/option_strings.h>

#include "core/download.h"
#include "core/manager.h"
#include "rpc/scgi.h"
#include "ui/root.h"
#include "rpc/parse.h"
#include "rpc/parse_commands.h"

#include "globals.h"
#include "control.h"
#include "command_helpers.h"

torrent::Object
apply_encryption(const torrent::Object::list_type& args) {
  uint32_t options_mask = torrent::net::NetworkConfig::encryption_none;

  for (const auto& arg : args) {
    uint32_t opt = torrent::option_find_string(torrent::OPTION_ENCRYPTION, arg.as_string().c_str());

    if (opt == torrent::net::NetworkConfig::encryption_none)
      options_mask = torrent::net::NetworkConfig::encryption_none;
    else
      options_mask |= opt;
  }

  torrent::config::network_config()->set_encryption_options(options_mask);

  return torrent::Object();
}

torrent::Object
apply_tos(const torrent::Object::string_type& arg) {
  rpc::command_base::value_type value;

  if (!rpc::parse_whole_value_nothrow(arg.c_str(), &value, 16, 1))
    value = torrent::option_find_string(torrent::OPTION_IP_TOS, arg.c_str());

  torrent::config::network_config()->set_priority(value);

  return torrent::Object();
}

torrent::Object apply_encoding_list(const std::string& arg) { torrent::encoding_list()->push_back(arg); return torrent::Object(); }

void
initialize_rpc_handlers() {
  rpc::rpc.initialize_handlers();

  unsigned int count = 0;

  for (const auto& [name, cmd] : rpc::commands) {
    if (!(cmd.m_flags & rpc::CommandMap::flag_public_rpc))
      continue;

    rpc::rpc.insert_command(name.c_str(), cmd.m_parm, cmd.m_doc);
    ++count;
  }

  lt_log_print(torrent::LOG_RPC_EVENTS, "RPC manager initialized with %u functions.", count);
}

torrent::Object
apply_scgi(const std::string& arg, int type) {
  if (worker_thread->scgi() != NULL)
    throw torrent::input_error("SCGI already enabled.");

  initialize_rpc_handlers();

  rpc::SCgi* scgi = new rpc::SCgi;

  rak::address_info* ai = NULL;
  torrent::sa_unique_ptr sa;

  try {
    int port, err;
    char dummy;
    char address[1024];
    std::string path;

    switch (type) {
    case 1:
      if (std::sscanf(arg.c_str(), ":%i%c", &port, &dummy) == 1) {
        sa = torrent::sa_make_inet();

        lt_log_print(torrent::LOG_RPC_EVENTS, "SCGI socket is open to any address and is a security risk");

      } else if (std::sscanf(arg.c_str(), "%1023[^:]:%i%c", address, &port, &dummy) == 2 ||
                 std::sscanf(arg.c_str(), "[%64[^]]]:%i%c", address, &port, &dummy) == 2) { // [xx::xx]:port format

        if ((err = rak::address_info::get_address_info(address, PF_UNSPEC, SOCK_STREAM, &ai)) != 0)
          throw torrent::input_error("Could not bind address: " + std::string(rak::address_info::strerror(err)) + ".");

        sa = torrent::sa_copy(ai->c_addrinfo()->ai_addr);

        lt_log_print(torrent::LOG_RPC_EVENTS, "SCGI socket is bound to an address and might be a security risk");

      } else {
        throw torrent::input_error("Could not parse address.");
      }

      if (port <= 0 || port >= (1 << 16))
        throw torrent::input_error("Invalid port number.");

      torrent::sap_set_port(sa, port);
      scgi->open_port(sa.get(), torrent::sap_length(sa), rpc::call_command_value("network.scgi.dont_route"));

      break;

    case 2:
    default:
      path = rak::path_expand(arg);

      unlink(path.c_str());
      scgi->open_named(path);
      break;
    }

    if (ai != NULL) rak::address_info::free_address_info(ai);

  } catch (torrent::local_error& e) {
    if (ai != NULL) rak::address_info::free_address_info(ai);

    delete scgi;
    throw torrent::input_error(e.what());
  }

  worker_thread->set_scgi(scgi);
  return torrent::Object();
}

torrent::Object
apply_xmlrpc_dialect(const std::string& arg) {
  int value;

  if (arg == "i8")
    value = rpc::XmlRpc::dialect_i8;
  else if (arg == "apache")
    value = rpc::XmlRpc::dialect_apache;
  else if (arg == "generic")
    value = rpc::XmlRpc::dialect_generic;
  else
    value = -1;

  rpc::rpc.set_dialect(value);
  return torrent::Object();
}

void
initialize_command_network() {
  auto cm = torrent::connection_manager();
  auto file_manager = torrent::file_manager();
  auto http_stack = torrent::net_thread::http_stack();
  auto network_config = torrent::config::network_config();

  CMD2_ANY_STRING  ("encoding.add", std::bind(&apply_encoding_list, std::placeholders::_2));

  // Isn't port_open used?
  CMD2_VAR_BOOL    ("network.port_open",   true);
  CMD2_VAR_BOOL    ("network.port_random", true);
  CMD2_VAR_STRING  ("network.port_range",  "6881-6999");

  CMD2_ANY         ("network.listen.port",        std::bind(&torrent::net::NetworkConfig::listen_port, network_config));
  CMD2_ANY         ("network.listen.backlog",     std::bind(&torrent::net::NetworkConfig::listen_backlog, network_config));
  CMD2_ANY_VALUE_V ("network.listen.backlog.set", std::bind(&torrent::ConnectionManager::set_listen_backlog, cm, std::placeholders::_2));

  CMD2_VAR_BOOL    ("protocol.pex",            true);
  CMD2_ANY_LIST    ("protocol.encryption.set", std::bind(&apply_encryption, std::placeholders::_2));

  CMD2_VAR_STRING  ("protocol.connection.leech", "leech");
  CMD2_VAR_STRING  ("protocol.connection.seed",  "seed");

  CMD2_VAR_STRING  ("protocol.choke_heuristics.up.leech", "upload_leech");
  CMD2_VAR_STRING  ("protocol.choke_heuristics.up.seed",  "upload_leech");
  CMD2_VAR_STRING  ("protocol.choke_heuristics.down.leech", "download_leech");
  CMD2_VAR_STRING  ("protocol.choke_heuristics.down.seed",  "download_leech");

  CMD2_ANY         ("network.http.cacert",                [http_stack](auto, auto) { return http_stack->http_cacert(); });
  CMD2_ANY_STRING_V("network.http.cacert.set",            [http_stack](auto, auto& str) { return http_stack->set_http_cacert(str); });
  CMD2_ANY         ("network.http.capath",                [http_stack](auto, auto) { return http_stack->http_capath(); });
  CMD2_ANY_STRING_V("network.http.capath.set",            [http_stack](auto, auto& str) { return http_stack->set_http_capath(str); });
  CMD2_ANY         ("network.http.dns_cache_timeout",     [http_stack](auto, auto) { return http_stack->dns_timeout(); });
  CMD2_ANY_VALUE_V ("network.http.dns_cache_timeout.set", [http_stack](auto, auto& value) { return http_stack->set_dns_timeout(value); });
  CMD2_ANY         ("network.http.current_open",          [http_stack](auto, auto) { return http_stack->active(); });
  CMD2_ANY         ("network.http.max_open",              [http_stack](auto, auto) { return http_stack->max_active(); });
  CMD2_ANY_VALUE_V ("network.http.max_open.set",          [http_stack](auto, auto& value) { return http_stack->set_max_active(value); });
  CMD2_ANY         ("network.http.proxy_address",         [http_stack](auto, auto) { return http_stack->http_proxy(); });
  CMD2_ANY_STRING_V("network.http.proxy_address.set",     [http_stack](auto, auto& str) { return http_stack->set_http_proxy(str); });
  CMD2_ANY         ("network.http.ssl_verify_host",       [http_stack](auto, auto) { return http_stack->ssl_verify_host(); });
  CMD2_ANY_VALUE_V ("network.http.ssl_verify_host.set",   [http_stack](auto, auto& value) { return http_stack->set_ssl_verify_host(value); });
  CMD2_ANY         ("network.http.ssl_verify_peer",       [http_stack](auto, auto) { return http_stack->ssl_verify_peer(); });
  CMD2_ANY_VALUE_V ("network.http.ssl_verify_peer.set",   [http_stack](auto, auto& value) { return http_stack->set_ssl_verify_peer(value); });

  CMD2_ANY         ("network.send_buffer.size",        std::bind(&torrent::net::NetworkConfig::send_buffer_size, network_config));
  CMD2_ANY_VALUE_V ("network.send_buffer.size.set",    std::bind(&torrent::net::NetworkConfig::set_send_buffer_size, network_config, std::placeholders::_2));
  CMD2_ANY         ("network.receive_buffer.size",     std::bind(&torrent::net::NetworkConfig::receive_buffer_size, network_config));
  CMD2_ANY_VALUE_V ("network.receive_buffer.size.set", std::bind(&torrent::net::NetworkConfig::set_receive_buffer_size, network_config, std::placeholders::_2));
  CMD2_ANY_STRING  ("network.tos.set",                 std::bind(&apply_tos, std::placeholders::_2));

  CMD2_ANY         ("network.bind_address",          std::bind(&torrent::net::NetworkConfig::bind_address_str, network_config));
  CMD2_ANY_STRING_V("network.bind_address.set",      std::bind(&core::Manager::set_bind_address, control->core(), std::placeholders::_2));
  CMD2_ANY         ("network.local_address",         std::bind(&torrent::net::NetworkConfig::local_address_str, network_config));
  CMD2_ANY_STRING_V("network.local_address.set",     std::bind(&core::Manager::set_local_address, control->core(), std::placeholders::_2));
  CMD2_ANY         ("network.proxy_address",         std::bind(&torrent::net::NetworkConfig::proxy_address_str, network_config));
  CMD2_ANY_STRING_V("network.proxy_address.set",     std::bind(&core::Manager::set_proxy_address, control->core(), std::placeholders::_2));

  CMD2_ANY         ("network.open_files",            std::bind(&torrent::FileManager::open_files, file_manager));
  CMD2_ANY         ("network.max_open_files",        std::bind(&torrent::FileManager::max_open_files, file_manager));
  CMD2_ANY_VALUE_V ("network.max_open_files.set",    std::bind(&torrent::FileManager::set_max_open_files, file_manager, std::placeholders::_2));
  CMD2_ANY         ("network.total_handshakes",      std::bind(&torrent::total_handshakes));
  CMD2_ANY         ("network.open_sockets",          std::bind(&torrent::ConnectionManager::size, cm));
  CMD2_ANY         ("network.max_open_sockets",      std::bind(&torrent::ConnectionManager::max_size, cm));
  CMD2_ANY_VALUE_V ("network.max_open_sockets.set",  std::bind(&torrent::ConnectionManager::set_max_size, cm, std::placeholders::_2));

  CMD2_ANY_STRING  ("network.scgi.open_port",        std::bind(&apply_scgi, std::placeholders::_2, 1));
  CMD2_ANY_STRING  ("network.scgi.open_local",       std::bind(&apply_scgi, std::placeholders::_2, 2));
  CMD2_VAR_BOOL    ("network.scgi.dont_route",       false);

  CMD2_ANY_STRING  ("network.xmlrpc.dialect.set",    [](const auto&, const auto& arg) { return apply_xmlrpc_dialect(arg); })
  CMD2_ANY         ("network.xmlrpc.size_limit",     [](const auto&, const auto&){ return rpc::rpc.size_limit(); });
  CMD2_ANY_VALUE_V ("network.xmlrpc.size_limit.set", [](const auto&, const auto& arg){ return rpc::rpc.set_size_limit(arg); });

  CMD2_VAR_BOOL    ("network.rpc.use_xmlrpc",        true);
  CMD2_VAR_BOOL    ("network.rpc.use_jsonrpc",       true);

  CMD2_ANY         ("network.block.ipv4",            std::bind(&torrent::net::NetworkConfig::is_block_ipv4, network_config));
  CMD2_ANY_VALUE_V ("network.block.ipv4.set",        std::bind(&torrent::net::NetworkConfig::set_block_ipv4, network_config, std::placeholders::_2));
  CMD2_ANY         ("network.block.ipv6",            std::bind(&torrent::net::NetworkConfig::is_block_ipv6, network_config));
  CMD2_ANY_VALUE_V ("network.block.ipv6.set",        std::bind(&torrent::net::NetworkConfig::set_block_ipv6, network_config, std::placeholders::_2));
  CMD2_ANY         ("network.block.ipv4in6",         std::bind(&torrent::net::NetworkConfig::is_block_ipv4in6, network_config));
  CMD2_ANY_VALUE_V ("network.block.ipv4in6.set",     std::bind(&torrent::net::NetworkConfig::set_block_ipv4in6, network_config, std::placeholders::_2));
  CMD2_ANY         ("network.prefer.ipv6",           std::bind(&torrent::net::NetworkConfig::is_prefer_ipv6, network_config));
  CMD2_ANY_VALUE_V ("network.prefer.ipv6.set",       std::bind(&torrent::net::NetworkConfig::set_prefer_ipv6, network_config, std::placeholders::_2));
}
