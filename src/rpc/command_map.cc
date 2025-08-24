#include "config.h"

#include <torrent/exceptions.h>
#include <torrent/object.h>
#include <torrent/data/file_list_iterator.h>

// Get better logging...
#include "globals.h"
#include "control.h"
#include "core/manager.h"

#include "command.h"
#include "command_map.h"

#include "rpc_manager.h"
#include "parse_commands.h"

namespace rpc {

command_base::stack_type command_base::current_stack;

CommandMap::iterator
CommandMap::insert(const key_type& key, int flags, const char* parm, const char* doc) {
  iterator itr = base_type::find(key);

  if (itr != base_type::end())
    throw torrent::internal_error("CommandMap::insert(...) tried to insert an already existing key.");

  // TODO: This is not honoring the public_xmlrpc flags!!!
  if (rpc::rpc.is_handlers_initialized() && (flags & flag_public_rpc))
    rpc::rpc.insert_command(key.c_str(), parm, doc);

  return base_type::insert(itr, value_type(key, command_map_data_type(flags, parm, doc)));
}

void
CommandMap::erase(iterator itr) {
  if (itr == end())
    return;

  // TODO: Remove the redirects instead...
  if (itr->second.m_flags & flag_has_redirects)
    throw torrent::input_error("Can't erase a command that has redirects.");

//   if (!(itr->second.m_flags & flag_dont_delete))
//     delete itr->second.m_variable;

  base_type::erase(itr);
}

void
CommandMap::create_redirect(const key_type& key_new, const key_type& key_dest, int flags) {
  iterator new_itr  = base_type::find(key_new);
  iterator dest_itr = base_type::find(key_dest);

  if (dest_itr == base_type::end())
    throw torrent::input_error("Tried to redirect to a key that doesn't exist: '" + std::string(key_dest) + "'.");

  if (new_itr != base_type::end())
    throw torrent::input_error("Tried to create a redirect key that already exists: '" + std::string(key_new) + "'.");

  if (dest_itr->second.m_flags & flag_is_redirect)
    throw torrent::input_error("Tried to redirect to a key that is not marked 'flag_is_redirect': '" +
                               std::string(key_dest) + "'.");

  dest_itr->second.m_flags |= flag_has_redirects;

  flags |= dest_itr->second.m_flags & ~(flag_has_redirects | flag_public_rpc);

  // TODO: This is not honoring the public_xmlrpc flags!!!
  if (rpc::rpc.is_handlers_initialized() && (flags & flag_public_rpc))
    rpc::rpc.insert_command(key_new.c_str(), dest_itr->second.m_parm, dest_itr->second.m_doc);

  iterator itr = base_type::insert(base_type::end(),
                                   value_type(key_new, command_map_data_type(flags,
                                                                             dest_itr->second.m_parm,
                                                                             dest_itr->second.m_doc)));

  // We can assume all the slots are the same size.
  itr->second.m_variable = dest_itr->second.m_variable;
  itr->second.m_anySlot = dest_itr->second.m_anySlot;
}

const CommandMap::mapped_type
CommandMap::call_catch(const key_type& key, const target_type& target, const mapped_type& args, const char* err) {
  try {
    return call_command(key, args, target);
  } catch (torrent::input_error& e) {
    control->core()->push_log((err + std::string(e.what())).c_str());
    return torrent::Object();
  }
}

const CommandMap::mapped_type
CommandMap::call_command(const key_type& key, const mapped_type& arg, const target_type& target) {
  iterator itr = base_type::find(key);

  if (itr == base_type::end())
    throw torrent::input_error("Command \"" + std::string(key) + "\" does not exist.");

  return itr->second.m_anySlot(&itr->second.m_variable, target, arg);
}

const CommandMap::mapped_type
CommandMap::call_command(iterator itr, const mapped_type& arg, const target_type& target) {
  return itr->second.m_anySlot(&itr->second.m_variable, target, arg);
}

}
