#include "config.h"

#include "xmlrpc.h"

#include "parse_commands.h"

#include <cstring>
#include <torrent/exceptions.h>

namespace rpc {

std::vector<std::unique_ptr<const char>> XmlRpc::m_command_names;

const char*
XmlRpc::store_command_name(const char* name) {
  if (::strnlen(name, 8192) == 8192)
    throw torrent::input_error("XMLRPC command name too long, limit is 8192 characters.");

  for (const auto& itr : m_command_names) {
    if (::strcmp(itr.get(), name) == 0)
      return itr.get();
  }

  m_command_names.push_back(std::unique_ptr<const char>(::strdup(name)));
  return m_command_names.back().get();
}

#ifndef HAVE_XMLRPC_C
#ifndef HAVE_XMLRPC_TINYXML2

void XmlRpc::initialize() {}
void XmlRpc::cleanup() {}

void XmlRpc::insert_command(const char*, const char*, const char*) {}
void XmlRpc::set_dialect(int) {}

bool XmlRpc::process(const char*, uint32_t, slot_write) { return false; }

int64_t XmlRpc::size_limit() { return 0; }
void    XmlRpc::set_size_limit(uint64_t size) {}

bool    XmlRpc::is_valid() const { return false; }

#endif
#endif

}
