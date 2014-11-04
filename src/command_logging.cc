// rTorrent - BitTorrent client
// Copyright (C) 2005-2011, Jari Sundell
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
#include <stdio.h>
#include <unistd.h>
#include <torrent/data/chunk_utils.h>
#include <torrent/object.h>
#include <torrent/utils/log.h>
#include <torrent/utils/option_strings.h>

#include "core/download.h"
#include "core/download_list.h"
#include "core/manager.h"
#include "rak/path.h"
#include "rpc/parse_commands.h"

#include "globals.h"
#include "control.h"
#include "command_helpers.h"

static const int log_flag_use_gz = 0x1;
static const int log_flag_append_pid = 0x2;

void
log_add_group_output_str(const char* group_name, const char* output_id) {
  int log_group = torrent::option_find_string(torrent::OPTION_LOG_GROUP, group_name);
  torrent::log_add_group_output(log_group, output_id);
}

torrent::Object
apply_log_open(int output_flags, const torrent::Object::list_type& args) {
  if (args.size() < 2)
    throw torrent::input_error("Invalid number of arguments.");
  
  torrent::Object::list_const_iterator itr = args.begin();

  std::string output_id = (itr++)->as_string();
  std::string file_name = rak::path_expand((itr++)->as_string());

  if ((output_flags & log_flag_append_pid)) {
    char buffer[32];
    snprintf(buffer, 32, ".%li", (long)getpid());

    file_name += buffer;
  }

  if ((output_flags & log_flag_use_gz))
    torrent::log_open_gz_file_output(output_id.c_str(), file_name.c_str());
  else
    torrent::log_open_file_output(output_id.c_str(), file_name.c_str());

  while (itr != args.end())
    log_add_group_output_str((itr++)->as_string().c_str(), output_id.c_str());

  return torrent::Object();
}

torrent::Object
apply_log_add_output(const torrent::Object::list_type& args) {
  if (args.size() != 2)
    throw torrent::input_error("Invalid number of arguments.");
  
  log_add_group_output_str(args.front().as_string().c_str(),
                           args.back().as_string().c_str());

  return torrent::Object();
}

// TODO: Deprecated.
torrent::Object
apply_log(const torrent::Object::string_type& arg, int logType) {
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

  if (!arg.empty()) {
    int logFd = open(rak::path_expand(arg).c_str(), O_WRONLY | O_APPEND | O_CREAT, 0644);

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
log_vmmap_dump(const std::string& str) {
  core::DownloadList* d_list = control->core()->download_list();
  std::vector<torrent::vm_mapping> all_mappings;

  for (core::DownloadList::iterator itr = d_list->begin(), last = d_list->end(); itr != last; itr++) {
    std::vector<torrent::vm_mapping> tmp_mappings = torrent::chunk_list_mapping((*itr)->download());

    all_mappings.insert(all_mappings.end(), tmp_mappings.begin(), tmp_mappings.end());    
  }

  FILE* log_file = fopen(str.c_str(), "w");

  for (std::vector<torrent::vm_mapping>::iterator itr = all_mappings.begin(), last = all_mappings.end(); itr != last; itr++) {
    fprintf(log_file, "%8p-%8p [%5llxk]\n", itr->ptr, (char*)itr->ptr + itr->length, (long long unsigned int)(itr->length / 1024));
  }

  fclose(log_file);
  return torrent::Object();
}

void
initialize_command_logging() {
  CMD2_ANY_LIST    ("log.open_file",        tr1::bind(&apply_log_open, 0, tr1::placeholders::_2));
  CMD2_ANY_LIST    ("log.open_gz_file",     tr1::bind(&apply_log_open, log_flag_use_gz, tr1::placeholders::_2));
  CMD2_ANY_LIST    ("log.open_file_pid",    tr1::bind(&apply_log_open, log_flag_append_pid, tr1::placeholders::_2));
  CMD2_ANY_LIST    ("log.open_gz_file_pid", tr1::bind(&apply_log_open, log_flag_append_pid | log_flag_use_gz, tr1::placeholders::_2));

  CMD2_ANY_LIST    ("log.add_output",   tr1::bind(&apply_log_add_output, tr1::placeholders::_2));

  CMD2_ANY_STRING  ("log.execute",    tr1::bind(&apply_log, tr1::placeholders::_2, 0));
  CMD2_ANY_STRING  ("log.vmmap.dump", tr1::bind(&log_vmmap_dump, tr1::placeholders::_2));
  CMD2_ANY_STRING_V("log.xmlrpc",     tr1::bind(&ThreadWorker::set_xmlrpc_log, worker_thread, tr1::placeholders::_2));
}
