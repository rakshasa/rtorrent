#include "config.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <torrent/data/chunk_utils.h>
#include <torrent/object.h>
#include <torrent/utils/log.h>
#include <torrent/utils/option_strings.h>

#include "control.h"
#include "command_helpers.h"
#include "globals.h"
#include "setup.h"
#include "core/download.h"
#include "core/download_list.h"
#include "core/manager.h"
#include "rpc/parse_commands.h"

torrent::Object
apply_log_open(int output_flags, const torrent::Object::list_type& raw_args) {
  std::vector<std::string> args;

  for (const auto& arg : raw_args)
    args.push_back(arg.as_string());

  apply_log_open_str(output_flags, args);
  return torrent::Object();
}

torrent::Object
apply_log_add_output(const torrent::Object::list_type& args) {
  if (args.size() != 2)
    throw torrent::input_error("Invalid number of arguments.");

  log_add_group_output_str(args.front().as_string(), args.back().as_string());

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
    int logFd = open(expand_path(arg).c_str(), O_WRONLY | O_APPEND | O_CREAT, 0644);

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
  std::vector<torrent::vm_mapping> all_mappings;

  for (const auto& d : *control->core()->download_list()) {
    std::vector<torrent::vm_mapping> tmp_mappings = torrent::chunk_list_mapping(d->download());

    all_mappings.insert(all_mappings.end(), tmp_mappings.begin(), tmp_mappings.end());
  }

  FILE* log_file = fopen(str.c_str(), "w");

  if (log_file == NULL)
    throw torrent::input_error("Could not open log file: " + str);

  for (auto& all_mapping : all_mappings) {
    fprintf(log_file, "%8p-%8p [%5llxk]\n", all_mapping.ptr, (char*)all_mapping.ptr + all_mapping.length, (long long unsigned int)(all_mapping.length / 1024));
  }

  fclose(log_file);
  return torrent::Object();
}

void
initialize_command_logging() {
  CMD2_ANY_LIST    ("log.open_file",          std::bind(&apply_log_open, 0, std::placeholders::_2));
  CMD2_ANY_LIST    ("log.open_file.flush",    std::bind(&apply_log_open, log_flag_flush, std::placeholders::_2));
  CMD2_ANY_LIST    ("log.open_gz_file",       std::bind(&apply_log_open, log_flag_use_gz, std::placeholders::_2));
  CMD2_ANY_LIST    ("log.open_file_pid",      std::bind(&apply_log_open, log_flag_append_pid, std::placeholders::_2));
  CMD2_ANY_LIST    ("log.open_gz_file_pid",   std::bind(&apply_log_open, log_flag_append_pid | log_flag_use_gz, std::placeholders::_2));
  CMD2_ANY_LIST    ("log.append_file",        std::bind(&apply_log_open, log_flag_append_file, std::placeholders::_2));
  CMD2_ANY_LIST    ("log.append_file.flush",  std::bind(&apply_log_open, log_flag_append_file | log_flag_flush, std::placeholders::_2));
  CMD2_ANY_LIST    ("log.append_gz_file",     std::bind(&apply_log_open, log_flag_append_file, std::placeholders::_2));

  CMD2_ANY_STRING_V("log.close",            std::bind(&torrent::log_close_output_str, std::placeholders::_2));

  CMD2_ANY_LIST    ("log.add_output",       std::bind(&apply_log_add_output, std::placeholders::_2));

  CMD2_ANY_STRING  ("log.execute",          std::bind(&apply_log, std::placeholders::_2, 0));
  CMD2_ANY_STRING  ("log.vmmap.dump",       std::bind(&log_vmmap_dump, std::placeholders::_2));
  CMD2_ANY_STRING_V("log.rpc",              [](const auto&, const auto& str) { scgi_thread::set_rpc_log(str); });

  CMD2_REDIRECT    ("log.xmlrpc", "log.rpc"); // For backwards compatibility
}
