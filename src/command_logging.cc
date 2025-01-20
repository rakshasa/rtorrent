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
static const int log_flag_append_file = 0x4;

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

  bool append = (output_flags & log_flag_append_file);

  if ((output_flags & log_flag_use_gz))
    torrent::log_open_gz_file_output(output_id.c_str(), file_name.c_str(), append);
  else
    torrent::log_open_file_output(output_id.c_str(), file_name.c_str(), append);

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
  CMD2_ANY_LIST    ("log.open_file",        std::bind(&apply_log_open, 0, std::placeholders::_2));
  CMD2_ANY_LIST    ("log.open_gz_file",     std::bind(&apply_log_open, log_flag_use_gz, std::placeholders::_2));
  CMD2_ANY_LIST    ("log.open_file_pid",    std::bind(&apply_log_open, log_flag_append_pid, std::placeholders::_2));
  CMD2_ANY_LIST    ("log.open_gz_file_pid", std::bind(&apply_log_open, log_flag_append_pid | log_flag_use_gz, std::placeholders::_2));
  CMD2_ANY_LIST    ("log.append_file",      std::bind(&apply_log_open, log_flag_append_file, std::placeholders::_2));
  CMD2_ANY_LIST    ("log.append_gz_file",   std::bind(&apply_log_open, log_flag_append_file, std::placeholders::_2));

  CMD2_ANY_STRING_V("log.close",      std::bind(&torrent::log_close_output_str, std::placeholders::_2));

  CMD2_ANY_LIST    ("log.add_output", std::bind(&apply_log_add_output, std::placeholders::_2));

  CMD2_ANY_STRING  ("log.execute",    std::bind(&apply_log, std::placeholders::_2, 0));
  CMD2_ANY_STRING  ("log.vmmap.dump", std::bind(&log_vmmap_dump, std::placeholders::_2));
  CMD2_ANY_STRING_V("log.rpc",        std::bind(&ThreadWorker::set_rpc_log, worker_thread, std::placeholders::_2));
  CMD2_REDIRECT    ("log.xmlrpc",     "log.rpc"); // For backwards compatibility
}
