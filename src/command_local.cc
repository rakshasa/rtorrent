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
#include <torrent/torrent.h>
#include <torrent/chunk_manager.h>

#include "core/download_list.h"
#include "core/download_store.h"
#include "core/manager.h"
#include "rpc/command_slot.h"
#include "rpc/command_variable.h"
#include "rpc/parse_commands.h"

#include "globals.h"
#include "control.h"
#include "command_helpers.h"

typedef torrent::ChunkManager CM_t;

torrent::Object
apply_execute_log(const torrent::Object& rawArgs) {
  if (rpc::execFile.log_fd() != -1) {
    ::close(rpc::execFile.log_fd());
    rpc::execFile.set_log_fd(-1);
  }

  if (rawArgs.is_string() && !rawArgs.as_string().empty()) {
    int logFd = open(rak::path_expand(rawArgs.as_string()).c_str(), O_WRONLY | O_APPEND | O_CREAT, 0644);

    if (logFd < 0)
      throw torrent::input_error("Could not open execute log file.");

    rpc::execFile.set_log_fd(logFd);
    control->core()->push_log("Opened execute log file.");

  } else {
    control->core()->push_log("Closed execute log file.");
  }

  return torrent::Object();
}

torrent::Object
apply_get_hostname() {
  char buffer[1024];

  if (gethostname(buffer, 1023) == -1)
    throw torrent::input_error("Unable to read hostname.");

//   if (shorten)
//     *std::find(buffer, buffer + 1023, '.') = '\0';

  return std::string(buffer);
}

void
initialize_command_local() {
  torrent::ChunkManager* chunkManager = torrent::chunk_manager();
  core::DownloadList*    dList = control->core()->download_list();
  core::DownloadStore*   dStore = control->core()->download_store();

  ADD_VARIABLE_C_STRING("client_version",  PACKAGE_VERSION);
  ADD_VARIABLE_C_STRING("library_version", torrent::version());

  ADD_COMMAND_VOID("get_hostname",         rak::ptr_fun(&apply_get_hostname));

  ADD_VARIABLE_VALUE("max_file_size", -1);
  ADD_VARIABLE_VALUE("split_file_size", -1);
  ADD_VARIABLE_STRING("split_suffix", ".part");

  ADD_COMMAND_VALUE_TRI("max_memory_usage",      rak::make_mem_fun(chunkManager, &CM_t::set_max_memory_usage), rak::make_mem_fun(chunkManager, &CM_t::max_memory_usage));
  ADD_COMMAND_VALUE_TRI("safe_sync",             rak::make_mem_fun(chunkManager, &CM_t::set_safe_sync), rak::make_mem_fun(chunkManager, &CM_t::safe_sync));
  ADD_COMMAND_VALUE_TRI("timeout_sync",          rak::make_mem_fun(chunkManager, &CM_t::set_timeout_sync), rak::make_mem_fun(chunkManager, &CM_t::timeout_sync));
  ADD_COMMAND_VALUE_TRI("timeout_safe_sync",     rak::make_mem_fun(chunkManager, &CM_t::set_timeout_safe_sync), rak::make_mem_fun(chunkManager, &CM_t::timeout_safe_sync));

  ADD_COMMAND_VALUE_TRI("preload_type",          rak::make_mem_fun(chunkManager, &CM_t::set_preload_type), rak::make_mem_fun(chunkManager, &CM_t::preload_type));
  ADD_COMMAND_VALUE_TRI("preload_min_size",      rak::make_mem_fun(chunkManager, &CM_t::set_preload_min_size), rak::make_mem_fun(chunkManager, &CM_t::preload_min_size));
  ADD_COMMAND_VALUE_TRI_KB("preload_required_rate", rak::make_mem_fun(chunkManager, &CM_t::set_preload_required_rate), rak::make_mem_fun(chunkManager, &CM_t::preload_required_rate));

  ADD_VARIABLE_STRING("directory", "./");

  ADD_COMMAND_STRING_TRI("session",            rak::make_mem_fun(dStore, &core::DownloadStore::set_path), rak::make_mem_fun(dStore, &core::DownloadStore::path));
  ADD_COMMAND_VOID("session_save",             rak::make_mem_fun(dList, &core::DownloadList::session_save));

  ADD_COMMAND_VALUE_TRI_OCT("umask",           rak::make_mem_fun(control, &Control::set_umask), rak::make_mem_fun(control, &Control::umask));
  ADD_COMMAND_STRING_TRI("working_directory",  rak::make_mem_fun(control, &Control::set_working_directory), rak::make_mem_fun(control, &Control::working_directory));

  ADD_COMMAND_LIST("execute",             rak::bind2_mem_fn(&rpc::execFile, &rpc::ExecFile::execute_object, rpc::ExecFile::flag_throw | rpc::ExecFile::flag_expand_tilde));
  ADD_COMMAND_LIST("execute_nothrow",     rak::bind2_mem_fn(&rpc::execFile, &rpc::ExecFile::execute_object, rpc::ExecFile::flag_expand_tilde));
  ADD_COMMAND_LIST("execute_raw",         rak::bind2_mem_fn(&rpc::execFile, &rpc::ExecFile::execute_object, rpc::ExecFile::flag_throw));
  ADD_COMMAND_LIST("execute_raw_nothrow", rak::bind2_mem_fn(&rpc::execFile, &rpc::ExecFile::execute_object, 0));

  ADD_COMMAND_STRING_UN("execute_log",    std::ptr_fun(&apply_execute_log));
}
