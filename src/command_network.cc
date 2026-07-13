#include "config.h"

#include <functional>
#include <cstdio>
#include <unistd.h>
#include <torrent/torrent.h>
#include <torrent/rate.h>
#include <torrent/data/file_manager.h>
#include <torrent/download/resource_manager.h>
#include <torrent/net/http_stack.h>
#include <torrent/net/socket_address.h>
#include <torrent/runtime/client_config.h>
#include <torrent/runtime/network_config.h>
#include <torrent/runtime/network_manager.h>
#include <torrent/runtime/proxy_manager.h>
#include <torrent/runtime/runtime.h>
#include <torrent/runtime/socket_manager.h>
#include <torrent/tracker/tracker.h>
#include <torrent/utils/log.h>
#include <torrent/utils/option_strings.h>

#include "globals.h"
#include "control.h"
#include "command_helpers.h"
#include "core/download.h"
#include "core/manager.h"
#include "rpc/scgi.h"
#include "ui/root.h"
#include "rpc/parse.h"
#include "rpc/parse_commands.h"

#ifdef HAVE_SYSTEMD
#include <sys/socket.h>
#include <systemd/sd-daemon.h>
#endif

torrent::Object
listen_port_range() {
  auto port_range = torrent::runtime::client_config()->listen_port_range();

  return std::to_string(port_range.first) + "-" + std::to_string(port_range.second);
}

void
set_listen_port_range(const std::string& arg) {
  unsigned int port_first{}, port_last{};

  if (std::sscanf(arg.c_str(), "%i-%i", &port_first, &port_last) != 2)
    throw torrent::input_error("Invalid port_range argument.");

  if (port_first >= (1 << 16) || port_last >= (1 << 16))
    throw torrent::input_error("Port range out-of-bounds.");

  torrent::runtime::client_config()->set_listen_port_range(port_first, port_last);
}

torrent::Object
get_encryption() {
  auto encryption_modes = torrent::runtime::network_config()->encryption_modes();

  return torrent::option_to_str_or_throw(torrent::OPTION_ENCRYPTION_HANDSHAKE, encryption_modes.first) + "," +
    torrent::option_to_str_or_throw(torrent::OPTION_ENCRYPTION_STREAM, encryption_modes.second);
}

torrent::Object
get_handshake_encryption() {
  auto encryption_modes = torrent::runtime::network_config()->encryption_modes();

  return torrent::option_to_str_or_throw(torrent::OPTION_ENCRYPTION_MODE, encryption_modes.first);
}

torrent::Object
get_stream_encryption() {
  auto encryption_modes = torrent::runtime::network_config()->encryption_modes();

  return torrent::option_to_str_or_throw(torrent::OPTION_ENCRYPTION_MODE, encryption_modes.second);
}

torrent::Object
apply_obsolete_encryption(const torrent::Object::list_type& args) {
  torrent::encryption_mode handshake_mode{torrent::ENCRYPTION_MODE_ALLOW};
  torrent::encryption_mode stream_mode{torrent::ENCRYPTION_MODE_ALLOW};

  for (auto& itr : args) {
    auto arg = itr.as_string();

    if (arg == "none") {
      handshake_mode = torrent::ENCRYPTION_MODE_DENY;
      stream_mode    = torrent::ENCRYPTION_MODE_DENY;
      break;

    } else if (arg == "allow_incoming") {
    } else if (arg == "try_outgoing") {
    } else if (arg == "require") {
      handshake_mode = torrent::ENCRYPTION_MODE_REQUIRE;

    } else if (arg == "require_RC4" || arg == "require_rc4") {
      handshake_mode = torrent::ENCRYPTION_MODE_REQUIRE;
      stream_mode    = torrent::ENCRYPTION_MODE_REQUIRE;
      break;

    } else if (arg == "enable_retry") {
    } else if (arg == "prefer_plaintext") {
    } else {
      throw torrent::input_error("Invalid encryption option: '" + arg + "'");
    }
  }

  lt_log_print(torrent::LOG_WARN, "Obsolete encryption options used, use 'handshake_{deny,allow,prefer,require}, stream_{deny,allow,prefer,require}' instead.");

  torrent::runtime::network_config()->set_encryption_modes(handshake_mode, stream_mode);
  return {};
}

torrent::Object
apply_encryption(const torrent::Object::list_type& args) {
  if (args.empty())
    throw torrent::input_error("No encryption options specified.");

  torrent::encryption_mode encryption_mode, handshake_mode, stream_mode;

  if (args.size() == 1) {
    try {
      encryption_mode = static_cast<torrent::encryption_mode>(torrent::option_find_string_str(torrent::OPTION_ENCRYPTION_MODE, args.front().as_string()));

    } catch (torrent::input_error& e) {
      return apply_obsolete_encryption(args);
    }

    torrent::runtime::network_config()->set_encryption_modes(encryption_mode, encryption_mode);
    return {};
  }

  if (args.size() != 2)
    return apply_obsolete_encryption(args);

  try {
    handshake_mode = static_cast<torrent::encryption_mode>(torrent::option_find_string_str(torrent::OPTION_ENCRYPTION_HANDSHAKE, args.front().as_string()));
    stream_mode    = static_cast<torrent::encryption_mode>(torrent::option_find_string_str(torrent::OPTION_ENCRYPTION_STREAM, args.back().as_string()));

  } catch (torrent::input_error& e) {
    return apply_obsolete_encryption(args);
  }

  torrent::runtime::network_config()->set_encryption_modes(handshake_mode, stream_mode);
  return {};
}

torrent::Object
apply_tos(const torrent::Object::string_type& arg) {
  rpc::command_base::value_type value;

  if (!rpc::parse_whole_value_nothrow(arg.c_str(), &value, 16, 1))
    value = torrent::option_find_string(torrent::OPTION_IP_TOS, arg.c_str());

  torrent::runtime::network_config()->set_priority(value);

  return torrent::Object();
}

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
  if (scgi_thread::scgi() != nullptr)
    throw torrent::input_error("SCGI already enabled.");

  initialize_rpc_handlers();

  torrent::sa_unique_ptr sa;

  auto scgi = std::make_unique<rpc::SCgi>();

  try {
    int port{};
    char dummy{};
    char address[1024];
    std::string path;

    switch (type) {
    case 1:
      if (std::sscanf(arg.c_str(), ":%i%c", &port, &dummy) == 1) {
        sa = torrent::sa_make_inet();

        lt_log_print(torrent::LOG_RPC_EVENTS, "SCGI socket is open to any address and is a security risk");

      } else if (std::sscanf(arg.c_str(), "%1023[^:]:%i%c", address, &port, &dummy) == 2 ||
                 std::sscanf(arg.c_str(), "[%64[^]]]:%i%c", address, &port, &dummy) == 2) { // [xx::xx]:port format

        try {
          sa = torrent::sa_copy(torrent::sa_lookup_address(address, AF_UNSPEC).get());
        } catch (torrent::input_error& e) {
          throw torrent::input_error("Could not bind address: " + std::string(e.what()));
        }

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
      path = expand_path(arg);

      unlink(path.c_str());
      scgi->open_named(path);
      break;
    }

  } catch (torrent::local_error& e) {
    throw torrent::input_error(e.what());
  }

  scgi_thread::set_scgi(scgi.release());
  return torrent::Object();
}

torrent::Object
apply_scgi_systemd() {
#ifdef HAVE_SYSTEMD
  if (scgi_thread::scgi() != nullptr)
    throw torrent::input_error("SCGI already enabled.");

  int n = sd_listen_fds(0);
  if (n < 1)
    throw torrent::input_error("No systemd socket(s) provided (sd_listen_fds returned " +
                               std::to_string(n) + ").");

  // Iterate over all provided fds. Use the first listening stream socket;
  // close the rest. The systemd docs say unused fds should be closed.
  int selected_fd = -1;
  for (int i = 0; i < n; i++) {
    int fd = SD_LISTEN_FDS_START + i;

    if (selected_fd != -1) {
      ::close(fd);
      continue;
    }

    auto err = sd_is_socket(fd, AF_UNSPEC, SOCK_STREAM, 1);

    if (err < 0) {
      // Safe to ignore errors here - we just skip it and move on.
      ::close(fd);
      continue;
    }

    if (err == 0) {
      // Not the socket we're looking for.
      ::close(fd);
      continue;
    }

    selected_fd = fd;
  }

  if (selected_fd == -1)
    throw torrent::input_error("No listening stream socket found among systemd-provided fds.");

  initialize_rpc_handlers();

  rpc::SCgi* scgi = new rpc::SCgi;
  scgi->open_fd(selected_fd);

  scgi_thread::set_scgi(scgi);
  return torrent::Object();
#else
  throw torrent::input_error("Systemd SCGI endpoint is not supported.");
#endif
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
  auto file_manager   = torrent::file_manager();
  auto http_stack     = torrent::net_thread::http_stack();
  auto nw_config      = torrent::runtime::network_config();

  CMD_ANY         ("network.listen.port",            [](auto, auto)        { return torrent::runtime::network_manager()->listen_port(); });
  CMD_ANY_VALUE_V ("network.listen.port.set",        [](auto, auto& value) { return torrent::runtime::network_manager()->set_listen_port(value); });
  CMD_ANY         ("network.listen.port.random",     [](auto, auto)        { return torrent::runtime::client_config()->listen_port_random(); });
  CMD_ANY_VALUE_V ("network.listen.port.random.set", [](auto, auto& value) { return torrent::runtime::client_config()->set_listen_port_random(value); });
  CMD_ANY         ("network.listen.port.range",      [](auto, auto)        { return listen_port_range(); });
  CMD_ANY_STRING_V("network.listen.port.range.set",  [](auto, auto& value) { return set_listen_port_range(value); });
  CMD_ANY         ("network.listen.backlog",         [](auto, auto)        { return torrent::runtime::network_config()->listen_backlog(); });
  CMD_ANY_VALUE_V ("network.listen.backlog.set",     [](auto, auto& value) { return torrent::runtime::network_config()->set_listen_backlog(value); });

  CMD_VAR_BOOL    ("protocol.pex",                   true);

  CMD_ANY_LIST    ("protocol.encryption",            [](auto, auto)        { return get_encryption(); });
  CMD_ANY_LIST    ("protocol.encryption.set",        [](auto, auto& args)  { return apply_encryption(args); });
  CMD_ANY_LIST    ("protocol.encryption.handshake",  [](auto, auto)        { return get_handshake_encryption(); });
  CMD_ANY_LIST    ("protocol.encryption.stream",     [](auto, auto)        { return get_stream_encryption(); });

  CMD_VAR_STRING  ("protocol.connection.leech",            "leech");
  CMD_VAR_STRING  ("protocol.connection.seed",             "seed");

  CMD_VAR_STRING  ("protocol.choke_heuristics.up.leech",   "upload_leech");
  CMD_VAR_STRING  ("protocol.choke_heuristics.up.seed",    "upload_leech");
  CMD_VAR_STRING  ("protocol.choke_heuristics.down.leech", "download_leech");
  CMD_VAR_STRING  ("protocol.choke_heuristics.down.seed",  "download_leech");

  CMD_ANY         ("network.http.cacert",                    [http_stack](auto, auto)        { return http_stack->http_cacert(); });
  CMD_ANY_STRING_V("network.http.cacert.set",                [http_stack](auto, auto& str)   { return http_stack->set_http_cacert(str); });
  CMD_ANY         ("network.http.capath",                    [http_stack](auto, auto)        { return http_stack->http_capath(); });
  CMD_ANY_STRING_V("network.http.capath.set",                [http_stack](auto, auto& str)   { return http_stack->set_http_capath(str); });
  CMD_ANY         ("network.http.dns_cache_timeout",         [http_stack](auto, auto)        { return http_stack->dns_timeout(); });
  CMD_ANY_VALUE_V ("network.http.dns_cache_timeout.set",     [http_stack](auto, auto& value) { return http_stack->set_dns_timeout(value); });
  CMD_ANY         ("network.http.current_open",              [http_stack](auto, auto)        { return http_stack->size(); });
  CMD_ANY         ("network.http.max_cache_connections",     [http_stack](auto, auto)        { return http_stack->max_cache_connections(); });
  CMD_ANY_VALUE_V ("network.http.max_cache_connections.set", [http_stack](auto, auto& value) { return http_stack->set_max_cache_connections(value); });
  CMD_ANY         ("network.http.max_host_connections",      [http_stack](auto, auto)        { return http_stack->max_host_connections(); });
  CMD_ANY_VALUE_V ("network.http.max_host_connections.set",  [http_stack](auto, auto& value) { return http_stack->set_max_host_connections(value); });
  CMD_ANY         ("network.http.max_total_connections",     [http_stack](auto, auto)        { return http_stack->max_total_connections(); });

  CMD_ANY         ("network.http.ssl_verify_host",           [http_stack](auto, auto)        { return http_stack->ssl_verify_host(); });
  CMD_ANY_VALUE_V ("network.http.ssl_verify_host.set",       [http_stack](auto, auto& value) { return http_stack->set_ssl_verify_host(value); });
  CMD_ANY         ("network.http.ssl_verify_peer",           [http_stack](auto, auto)        { return http_stack->ssl_verify_peer(); });
  CMD_ANY_VALUE_V ("network.http.ssl_verify_peer.set",       [http_stack](auto, auto& value) { return http_stack->set_ssl_verify_peer(value); });

  CMD_ANY         ("network.send_buffer.size",               [nw_config](auto, auto)         { return nw_config->send_buffer_size(); });
  CMD_ANY_VALUE_V ("network.send_buffer.size.set",           [nw_config](auto, auto& value)  { return nw_config->set_send_buffer_size(value); });
  CMD_ANY         ("network.receive_buffer.size",            [nw_config](auto, auto)         { return nw_config->receive_buffer_size(); });
  CMD_ANY_VALUE_V ("network.receive_buffer.size.set",        [nw_config](auto, auto& value)  { return nw_config->set_receive_buffer_size(value); });
  CMD_ANY_STRING  ("network.tos.set",                        [](auto, auto& str)             { return apply_tos(str); });

  CMD_ANY         ("network.bind_address",                   [nw_config](auto, auto)         { return nw_config->bind_address_best_match_str(); });
  CMD_ANY_STRING_V("network.bind_address.set",               [nw_config](auto, auto& str)    { return nw_config->set_bind_address_str(str); });
  CMD_ANY         ("network.bind_address.ipv4",              [nw_config](auto, auto)         { return nw_config->bind_inet_address_str(); });
  CMD_ANY_STRING_V("network.bind_address.ipv4.set",          [nw_config](auto, auto& str)    { return nw_config->set_bind_inet_address_str(str); });
  CMD_ANY         ("network.bind_address.ipv6",              [nw_config](auto, auto)         { return nw_config->bind_inet6_address_str(); });
  CMD_ANY_STRING_V("network.bind_address.ipv6.set",          [nw_config](auto, auto& str)    { return nw_config->set_bind_inet6_address_str(str); });

  CMD_ANY         ("network.local_address",                  [nw_config](auto, auto)         { return nw_config->local_address_best_match_str(); });
  CMD_ANY_STRING_V("network.local_address.set",              [nw_config](auto, auto& str)    { return nw_config->set_local_address_str(str); });
  CMD_ANY         ("network.local_address.ipv4",             [nw_config](auto, auto)         { return nw_config->local_inet_address_str(); });
  CMD_ANY_STRING_V("network.local_address.ipv4.set",         [nw_config](auto, auto& str)    { return nw_config->set_local_inet_address_str(str); });
  CMD_ANY         ("network.local_address.ipv6",             [nw_config](auto, auto)         { return nw_config->local_inet6_address_str(); });
  CMD_ANY_STRING_V("network.local_address.ipv6.set",         [nw_config](auto, auto& str)    { return nw_config->set_local_inet6_address_str(str); });

  CMD_ANY         ("network.proxy.global",                   [](auto, auto)                  { return torrent::runtime::proxy_manager()->proxy_url(); });
  CMD_ANY_STRING_V("network.proxy.global.set",               [](auto, auto& str)             { return torrent::runtime::proxy_manager()->set_proxy_url(str); });
  CMD_ANY         ("network.proxy.http",                     [](auto, auto)                  { return torrent::runtime::proxy_manager()->http_proxy_url(); });
  CMD_ANY_STRING_V("network.proxy.http.set",                 [](auto, auto& str)             { return torrent::runtime::proxy_manager()->set_http_proxy_url(str); });

  CMD_ANY         ("network.open_files",                     [file_manager](auto, auto)      { return file_manager->open_files(); });
  CMD_ANY         ("network.max_open_files",                 [file_manager](auto, auto)      { return file_manager->max_open_files(); });
  CMD_ANY         ("network.total_handshakes",               [](auto, auto)                  { return torrent::runtime::total_handshakes(); });

  CMD_ANY_STRING  ("network.scgi.open_port",                 [](auto, auto& arg)             { return apply_scgi(arg, 1); });
  CMD_ANY_STRING  ("network.scgi.open_local",                [](auto, auto& arg)             { return apply_scgi(arg, 2); });
  CMD_VAR_BOOL    ("network.scgi.dont_route",                false);
  CMD_ANY         ("network.scgi.open_systemd",              [](auto, auto)                  { return apply_scgi_systemd(); });

  CMD_ANY         ("network.scgi.use_gzip",                  [](auto, auto)                  { return rpc::rpc.scgi_allow_compression(); });
  CMD_ANY_VALUE_V ("network.scgi.use_gzip.set",              [](auto, auto& arg)             { return rpc::rpc.set_scgi_allow_compression(arg); });
  CMD_ANY         ("network.scgi.gzip.min_size",             [](auto, auto)                  { return rpc::rpc.scgi_min_compress_size(); });
  CMD_ANY_VALUE_V ("network.scgi.gzip.min_size.set",         [](auto, auto& arg)             { return rpc::rpc.set_scgi_min_compress_size(arg); });

  CMD_ANY_STRING  ("network.xmlrpc.dialect.set",             [](auto, auto& arg)             { return apply_xmlrpc_dialect(arg); })
  CMD_ANY         ("network.xmlrpc.size_limit",              [](auto, auto)                  { return rpc::rpc.size_limit(); });
  CMD_ANY_VALUE_V ("network.xmlrpc.size_limit.set",          [](auto, auto& arg)             { return rpc::rpc.set_size_limit(arg); });

  CMD_VAR_BOOL    ("network.rpc.use_xmlrpc",                 true);
  CMD_VAR_BOOL    ("network.rpc.use_jsonrpc",                true);

  CMD_ANY         ("network.block.ipv4",                     [nw_config](auto, auto)         { return nw_config->is_block_ipv4(); });
  CMD_ANY_VALUE_V ("network.block.ipv4.set",                 [nw_config](auto, auto& value)  { return nw_config->set_block_ipv4(value); });
  CMD_ANY         ("network.block.ipv6",                     [nw_config](auto, auto)         { return nw_config->is_block_ipv6(); });
  CMD_ANY_VALUE_V ("network.block.ipv6.set",                 [nw_config](auto, auto& value)  { return nw_config->set_block_ipv6(value); });
  CMD_ANY         ("network.block.ipv4in6",                  [nw_config](auto, auto)         { return nw_config->is_block_ipv4in6(); });
  CMD_ANY_VALUE_V ("network.block.ipv4in6.set",              [nw_config](auto, auto& value)  { return nw_config->set_block_ipv4in6(value); });
  CMD_ANY         ("network.block.outgoing",                 [nw_config](auto, auto)         { return nw_config->is_block_outgoing(); });
  CMD_ANY_VALUE_V ("network.block.outgoing.set",             [nw_config](auto, auto& value)  { return nw_config->set_block_outgoing(value); });
  CMD_ANY         ("network.prefer.ipv6",                    [nw_config](auto, auto)         { return nw_config->is_prefer_ipv6(); });
  CMD_ANY_VALUE_V ("network.prefer.ipv6.set",                [nw_config](auto, auto& value)  { return nw_config->set_prefer_ipv6(value); });

  rpc::rpc.mark_safe("network.port_open");
  rpc::rpc.mark_safe("network.port_random");
  rpc::rpc.mark_safe("network.port_range");
  rpc::rpc.mark_safe("network.listen.port");
  rpc::rpc.mark_safe("network.listen.backlog");

  rpc::rpc.mark_safe("network.http.current_open");
  rpc::rpc.mark_safe("network.http.max_cache_connections");
  rpc::rpc.mark_safe("network.http.max_host_connections");
  rpc::rpc.mark_safe("network.http.max_total_connections");

  rpc::rpc.mark_safe("network.total_handshakes");
  rpc::rpc.mark_safe("network.open_files");
  rpc::rpc.mark_safe("network.max_open_files");

  rpc::rpc.mark_safe("network.send_buffer.size");
  rpc::rpc.mark_safe("network.receive_buffer.size");
  rpc::rpc.mark_safe("network.bind_address");
  rpc::rpc.mark_safe("network.local_address");
  rpc::rpc.mark_safe("network.xmlrpc.size_limit");
  rpc::rpc.mark_safe("network.open_sockets");

  rpc::rpc.mark_safe("network.http.cacert");
  rpc::rpc.mark_safe("network.http.capath");
  rpc::rpc.mark_safe("network.proxy.global");
  rpc::rpc.mark_safe("network.proxy.http");
  rpc::rpc.mark_safe("network.scgi.dont_route");

  rpc::rpc.mark_safe("protocol.pex");

  rpc::rpc.mark_safe("network.rpc.use_xmlrpc");
  rpc::rpc.mark_safe("network.rpc.use_jsonrpc");
}
