#include "config.h"

#include "xmlrpc.h"

#include "parse_commands.h"

#include <torrent/exceptions.h>

namespace rpc {

#ifndef HAVE_XMLRPC_C
#ifndef HAVE_XMLRPC_TINYXML2

void XmlRpc::initialize() {}
void XmlRpc::cleanup() {}

void XmlRpc::insert_command(__UNUSED const char* name, __UNUSED const char* parm, __UNUSED const char* doc) {}
void XmlRpc::set_dialect(__UNUSED int dialect) {}

bool XmlRpc::process(__UNUSED const char* inBuffer, __UNUSED uint32_t length, __UNUSED slot_write slotWrite) { return false; }

int64_t XmlRpc::size_limit() { return 0; }
void    XmlRpc::set_size_limit(uint64_t size) {}

bool    XmlRpc::is_valid() const { return false; }

#endif
#endif

}
