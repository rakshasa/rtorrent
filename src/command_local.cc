// rTorrent - BitTorrent client
// Copyright (C) 2005-2007, Jari Sundell
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// In addition, as a special exception, the copyright holders give
// permission to link the code of portions of this program with the
// OpenSSL library under certain conditions as described in each
// individual source file, and distribute linked combinations
// including the two.
//
// You must obey the GNU General Public License in all respects for
// all of the code used other than OpenSSL.  If you modify file(s)
// with this exception, you may extend this exception to your version
// of the file(s), but you are not obligated to do so.  If you do not
// wish to do so, delete this exception statement from your version.
// If you delete this exception statement from all source files in the
// program, then also delete it here.
//
// Contact:  Jari Sundell <jaris@ifi.uio.no>
//
//           Skomakerveien 33
//           3185 Skoppum, NORWAY

#include "config.h"

#include <fcntl.h>
#include <functional>
#include <unistd.h>
#include <rak/path.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <torrent/torrent.h>
#include <torrent/chunk_manager.h>
#include <torrent/data/file_manager.h>

#include "core/download_list.h"
#include "core/download_store.h"
#include "core/manager.h"
#include "rak/string_manip.h"
#include "rpc/command_slot.h"
#include "rpc/command_variable.h"
#include "rpc/parse_commands.h"
#include "rpc/scgi.h"
#include "utils/file_status_cache.h"

#include "globals.h"
#include "control.h"
#include "command_helpers.h"

typedef torrent::ChunkManager CM_t;
typedef torrent::FileManager  FM_t;

torrent::Object
apply_log(int logType, const torrent::Object& rawArgs) {
  if (rpc::execFile.log_fd() != -1) {
    switch (logType) {
    case 0: ::close(rpc::execFile.log_fd()); rpc::execFile.set_log_fd(-1); break;
    case 1:
//       if (control->scgi()) {
//         ::close(control->scgi()->log_fd());
//         control->scgi()->set_log_fd(-1);
//       }
      break;
    default: break;
    }
  }

  if (rawArgs.is_string() && !rawArgs.as_string().empty()) {
    int logFd = open(rak::path_expand(rawArgs.as_string()).c_str(), O_WRONLY | O_APPEND | O_CREAT, 0644);

    if (logFd < 0)
      throw torrent::input_error("Could not open execute log file.");

    switch (logType) {
    case 0: rpc::execFile.set_log_fd(logFd); break;
//     case 1: if (control->scgi()) control->scgi()->set_log_fd(logFd); break;
    default: break;
    }

    control->core()->push_log("Opened log file.");

  } else {
    control->core()->push_log("Closed log file.");
  }

  return torrent::Object();
}

torrent::Object
system_hostname() {
  char buffer[1024];

  if (gethostname(buffer, 1023) == -1)
    throw torrent::input_error("Unable to read hostname.");

//   if (shorten)
//     *std::find(buffer, buffer + 1023, '.') = '\0';

  return std::string(buffer);
}

torrent::Object
system_get_cwd() {
  char* buffer = getcwd(NULL, 0);

  if (buffer == NULL)
    throw torrent::input_error("Unable to read cwd.");

  torrent::Object result = torrent::Object(std::string(buffer));
  free(buffer);

  return result;
}

torrent::Object
system_set_cwd(const torrent::Object& rawArgs) {
  if (::chdir(rawArgs.as_string().c_str()) != 0)
    throw torrent::input_error("Could not change current working directory.");

  return torrent::Object();
}

inline torrent::Object::list_const_iterator
post_increment(torrent::Object::list_const_iterator& itr, const torrent::Object::list_const_iterator& last) {
  if (itr == last)
    throw torrent::input_error("Invalid number of arguments.");

  return itr++;
}

inline const std::string&
check_name(const std::string& str) {
  if (!rak::is_all_name(str))
    throw torrent::input_error("Non-alphanumeric characters found.");

  return str;
}  

torrent::Object
group_insert(__UNUSED rpc::target_type target, const torrent::Object& rawArgs) {
  torrent::Object::list_const_iterator itr = rawArgs.as_list().begin();
  torrent::Object::list_const_iterator last = rawArgs.as_list().end();

  const std::string& name = check_name(post_increment(itr, last)->as_string());
  const std::string& view = check_name(post_increment(itr, last)->as_string());

  rpc::commands.call("method.insert", rpc::create_object_list("group." + name + ".view", "string", view));

  rpc::commands.call("method.insert", rpc::create_object_list("group." + name + ".ratio.enable", "simple", "schedule=group." + name + ".ratio,5,60,on_ratio=" + name));
  rpc::commands.call("method.insert", rpc::create_object_list("group." + name + ".ratio.disable", "simple", "schedule_remove=group." + name + ".ratio"));
  rpc::commands.call("method.insert", rpc::create_object_list("group." + name + ".ratio.command", "simple", "d.try_close= ;d.set_ignore_commands=1"));
  rpc::commands.call("method.insert", rpc::create_object_list("group." + name + ".ratio.min", "value", (int64_t)200));
  rpc::commands.call("method.insert", rpc::create_object_list("group." + name + ".ratio.max", "value", (int64_t)300));
  rpc::commands.call("method.insert", rpc::create_object_list("group." + name + ".ratio.upload", "value", (int64_t)20 << 20));

  return name;
}

void
initialize_command_local() {
  torrent::ChunkManager* chunkManager = torrent::chunk_manager();
  torrent::FileManager*  fileManager = torrent::file_manager();
  core::DownloadList*    dList = control->core()->download_list();
  core::DownloadStore*   dStore = control->core()->download_store();

  ADD_COMMAND_VOID("system.hostname",            rak::ptr_fun(&system_hostname));
  ADD_COMMAND_VOID("system.pid",                 rak::ptr_fun(&getpid));

  rpc::commands.call("method.insert", rpc::create_object_list("system.client_version", "string|static|const", PACKAGE_VERSION));
  rpc::commands.call("method.insert", rpc::create_object_list("system.library_version", "string|static|const", torrent::version()));
  rpc::commands.call("method.insert", rpc::create_object_list("system.file.allocate", "value|const", (int64_t)0));
  rpc::commands.call("method.insert", rpc::create_object_list("system.file.max_size", "value|const", -1));
  rpc::commands.call("method.insert", rpc::create_object_list("system.file.split_size", "value|const", -1));
  rpc::commands.call("method.insert", rpc::create_object_list("system.file.split_suffix", "string|const", ".part"));
  rpc::commands.call("method.insert", rpc::create_object_list("system.session_name", "string|const", ""));

  ADD_COMMAND_VOID("system.file_status_cache.size",  rak::make_mem_fun((utils::FileStatusCache::base_type*)control->core()->file_status_cache(),
                                                                       &utils::FileStatusCache::size));
  ADD_COMMAND_VOID("system.file_status_cache.prune", rak::make_mem_fun(control->core()->file_status_cache(), &utils::FileStatusCache::prune));

  ADD_COMMAND_VOID("system.files.opened_counter",    rak::make_mem_fun(fileManager, &FM_t::files_opened_counter));
  ADD_COMMAND_VOID("system.files.closed_counter",    rak::make_mem_fun(fileManager, &FM_t::files_closed_counter));
  ADD_COMMAND_VOID("system.files.failed_counter",    rak::make_mem_fun(fileManager, &FM_t::files_failed_counter));

  ADD_COMMAND_VOID("system.time",                    rak::make_mem_fun(&cachedTime, &rak::timer::seconds));
  ADD_COMMAND_VOID("system.time_seconds",            rak::ptr_fun(&rak::timer::current_seconds));
  ADD_COMMAND_VOID("system.time_usec",               rak::ptr_fun(&rak::timer::current_usec));

  ADD_COMMAND_VALUE_SET_OCT("system.", "umask",      std::ptr_fun(&umask));
  ADD_COMMAND_STRING_PREFIX("system.", "cwd",        std::ptr_fun(system_set_cwd), rak::ptr_fun(&system_get_cwd));

  ADD_COMMAND_VOID("pieces.sync.always_safe",          rak::make_mem_fun(chunkManager, &CM_t::safe_sync));
  ADD_COMMAND_VALUE_UN("pieces.sync.always_safe.set",  rak::make_mem_fun(chunkManager, &CM_t::set_safe_sync));
  ADD_COMMAND_VOID("pieces.sync.safe_free_diskspace",  rak::make_mem_fun(chunkManager, &CM_t::safe_free_diskspace));
  ADD_COMMAND_VOID("pieces.sync.timeout",              rak::make_mem_fun(chunkManager, &CM_t::timeout_sync));
  ADD_COMMAND_VALUE_UN("pieces.sync.timeout.set",      rak::make_mem_fun(chunkManager, &CM_t::set_timeout_sync));
  ADD_COMMAND_VOID("pieces.sync.timeout_safe",         rak::make_mem_fun(chunkManager, &CM_t::timeout_safe_sync));
  ADD_COMMAND_VALUE_UN("pieces.sync.timeout_safe.set", rak::make_mem_fun(chunkManager, &CM_t::set_timeout_safe_sync));

  ADD_COMMAND_VOID("pieces.preload.type",              rak::make_mem_fun(chunkManager, &CM_t::preload_type));
  ADD_COMMAND_VALUE_UN("pieces.preload.type.set",      rak::make_mem_fun(chunkManager, &CM_t::set_preload_type));
  ADD_COMMAND_VOID("pieces.preload.min_size",          rak::make_mem_fun(chunkManager, &CM_t::preload_min_size));
  ADD_COMMAND_VALUE_UN("pieces.preload.min_size.set",  rak::make_mem_fun(chunkManager, &CM_t::set_preload_min_size));
  ADD_COMMAND_VOID("pieces.preload.min_rate",          rak::make_mem_fun(chunkManager, &CM_t::preload_required_rate));
  ADD_COMMAND_VALUE_UN("pieces.preload.min_rate.set",  rak::make_mem_fun(chunkManager, &CM_t::set_preload_required_rate));

  ADD_COMMAND_VOID("pieces.memory.current",      rak::make_mem_fun(chunkManager, &CM_t::memory_usage));
  ADD_COMMAND_VOID("pieces.memory.max",          rak::make_mem_fun(chunkManager, &CM_t::max_memory_usage));
  ADD_COMMAND_VALUE_UN("pieces.memory.max.set",  rak::make_mem_fun(chunkManager, &CM_t::set_max_memory_usage));
  ADD_COMMAND_VOID("pieces.stats_preloaded",     rak::make_mem_fun(chunkManager, &CM_t::stats_preloaded));
  ADD_COMMAND_VOID("pieces.stats_not_preloaded", rak::make_mem_fun(chunkManager, &CM_t::stats_not_preloaded));


  ADD_VARIABLE_STRING("directory", "./");

  ADD_COMMAND_STRING_TRI("session",            rak::make_mem_fun(dStore, &core::DownloadStore::set_path), rak::make_mem_fun(dStore, &core::DownloadStore::path));
  ADD_COMMAND_VOID("session_save",             rak::make_mem_fun(dList, &core::DownloadList::session_save));

  ADD_COMMAND_LIST("execute",             rak::bind2_mem_fn(&rpc::execFile, &rpc::ExecFile::execute_object, rpc::ExecFile::flag_throw | rpc::ExecFile::flag_expand_tilde));
  ADD_COMMAND_LIST("execute_nothrow",     rak::bind2_mem_fn(&rpc::execFile, &rpc::ExecFile::execute_object, rpc::ExecFile::flag_expand_tilde));
  ADD_COMMAND_LIST("execute_raw",         rak::bind2_mem_fn(&rpc::execFile, &rpc::ExecFile::execute_object, rpc::ExecFile::flag_throw));
  ADD_COMMAND_LIST("execute_raw_nothrow", rak::bind2_mem_fn(&rpc::execFile, &rpc::ExecFile::execute_object, 0));
  ADD_COMMAND_LIST("execute_capture",     rak::bind2_mem_fn(&rpc::execFile, &rpc::ExecFile::execute_object, rpc::ExecFile::flag_throw | rpc::ExecFile::flag_expand_tilde | rpc::ExecFile::flag_capture));
  ADD_COMMAND_LIST("execute_capture_nothrow", rak::bind2_mem_fn(&rpc::execFile, &rpc::ExecFile::execute_object, rpc::ExecFile::flag_expand_tilde | rpc::ExecFile::flag_capture));

  ADD_COMMAND_STRING("log.execute", rak::bind_ptr_fn(&apply_log, 0));
  ADD_COMMAND_STRING("log.xmlrpc",  rak::bind_ptr_fn(&apply_log, 1));

  *rpc::Command::argument(0) = "placeholder.0";
  *rpc::Command::argument(1) = "placeholder.1";
  *rpc::Command::argument(2) = "placeholder.2";
  *rpc::Command::argument(3) = "placeholder.3";
  CMD_OBJ_P("argument.0", get_generic, rpc::Command::argument(0));
  CMD_OBJ_P("argument.1", get_generic, rpc::Command::argument(1));
  CMD_OBJ_P("argument.2", get_generic, rpc::Command::argument(2));
  CMD_OBJ_P("argument.3", get_generic, rpc::Command::argument(3));

  CMD_N_LIST("group.insert", rak::ptr_fn(&group_insert));
}
