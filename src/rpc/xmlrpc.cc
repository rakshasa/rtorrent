#include "config.h"

#include "xmlrpc.h"

#include "parse_commands.h"

#include <torrent/exceptions.h>

namespace rpc {

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
