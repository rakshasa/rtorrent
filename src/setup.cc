#include "config.h"

#include "setup.h"

#include <fstream>
#include <unistd.h>
#include <torrent/exceptions.h>

#include "control.h"
#include "globals.h"
#include "option_parser.h"
#include "core/download_factory.h"
#include "rpc/parse_commands.h"
#include "session/download_storer.h"
#include "utils/directory.h"

void do_panic(int signum);
void print_help();

int
parse_main_options(int argc, char** argv) {
  try {
    OptionParser optionParser;

    // Converted.
    optionParser.insert_flag('h', [](auto&) { print_help(); });
    optionParser.insert_flag('n', [](auto&) { });
    optionParser.insert_flag('D', [](auto&) { });
    optionParser.insert_flag('I', [](auto&) { });
    optionParser.insert_flag('K', [](auto&) { });

    optionParser.insert_option('b', [](auto& arg) { rpc::call_command_set_string("network.bind_address.set", arg); });
    optionParser.insert_option('d', [](auto& arg) { rpc::call_command_set_string("directory.default.set", arg); });
    optionParser.insert_option('i', [](auto& arg) { rpc::call_command_set_string("ip", arg); });
    optionParser.insert_option('p', [](auto& arg) { rpc::call_command_set_string("network.port_range.set", arg); });
    optionParser.insert_option('s', [](auto& arg) { rpc::call_command_set_string("session", arg); });

    optionParser.insert_option('O',      [](auto& arg) { rpc::parse_command_single_std(arg); });
    optionParser.insert_option_list('o', [](auto& arg1, auto& arg2) { rpc::call_command_set_std_string(arg1, arg2); });

    return optionParser.process(argc, argv);

  } catch (torrent::input_error& e) {
    throw torrent::input_error("Failed to parse command line option: " + std::string(e.what()));
  }
}

void
parse_config_file(int argc, char** argv, std::function<void (const std::string&)> parse_fn) {
  if (OptionParser::has_flag('n', argc, argv))
    return parse_fn("");

  char* config_dir = std::getenv("XDG_CONFIG_HOME");
  char* home_dir = std::getenv("HOME");

  if (config_dir != nullptr && config_dir[0] == '/' && access((std::string(config_dir) + "/rtorrent/rtorrent.rc").c_str(), F_OK) != -1)
    return parse_fn(std::string(config_dir) + "/rtorrent/rtorrent.rc");

  if (home_dir == nullptr)
    throw torrent::input_error("Could not find home directory, HOME environment variable not set.");

  if (access((std::string(home_dir) + "/.config/rtorrent/rtorrent.rc").c_str(), F_OK) != -1)
    return parse_fn(std::string(home_dir) + "/.config/rtorrent/rtorrent.rc");

  return parse_fn(std::string(home_dir) + "/.rtorrent.rc");
}

// TODO: Add to header?
static const int log_flag_use_gz = 0x1;
static const int log_flag_append_pid = 0x2;
static const int log_flag_append_file = 0x4;
static const int log_flag_flush = 0x8;

void log_add_group_output_str(const std::string& group_name, const std::string& output_id);
void apply_log_open_str(int output_flags, const std::vector<std::string>& args);

void
config_comment_log(const std::string& command, const std::string& raw_args) {
  std::vector<std::string> args;
  size_t                   pos{};

  while (pos < raw_args.size()) {
    size_t next_pos = raw_args.find(',', pos);

    if (next_pos == std::string::npos)
      next_pos = raw_args.size();

    args.push_back(raw_args.substr(pos, next_pos - pos));
    pos = next_pos + 1;
  }

  if (command == "log.add_output")
    log_add_group_output_str(args[0], args[1]);
  else if (command == "log.open_file")
    apply_log_open_str(0, args);
  else if (command == "log.open_file.flush")
    apply_log_open_str(log_flag_flush, args);
  else if (command == "log.open_gz_file")
    apply_log_open_str(log_flag_use_gz, args);
  else if (command == "log.open_file_pid")
    apply_log_open_str(log_flag_append_pid, args);
  else if (command == "log.open_gz_file_pid")
    apply_log_open_str(log_flag_append_pid | log_flag_use_gz, args);
  else if (command == "log.append_file")
    apply_log_open_str(log_flag_append_file, args);
  else if (command == "log.append_file.flush")
    apply_log_open_str(log_flag_append_file | log_flag_flush, args);
  else if (command == "log.append_gz_file")
    apply_log_open_str(log_flag_append_file | log_flag_use_gz, args);
  else
    throw torrent::input_error("Unknown log command: " + command);
}

// Call special commands in the format "# do:command=args" in the config file.
void
parse_config_file_comments(const std::string& path) {
  std::fstream file(path, std::ios::in);

  if (!file.is_open())
    return;

  std::string line;

  while (std::getline(file, line)) {
    if (line.size() <= 5 || line.compare(0, 5, "# do:") != 0)
      continue;

    auto equal_pos = line.find('=');

    if (equal_pos == std::string::npos)
      throw torrent::input_error("Invalid command in config file comment: " + line);

    std::string command = line.substr(5, equal_pos - 5);
    std::string args    = line.substr(equal_pos + 1);

    if (command.empty())
      throw torrent::input_error("Invalid command in config file comment: " + line);

    if (command.compare(0, 4, "log.") == 0) {
      config_comment_log(command, args);
      continue;
    }
  }
}

void
load_session_torrents(const std::string& path) {
  auto entries = session::DownloadStorer::get_formated_entries(path);

  for (const auto& entry : entries) {
    // We don't really support session torrents that are links. These
    // would be overwritten anyway on exit, and thus not really be
    // useful.
    if (!entry.is_file())
      continue;

    auto* f = new core::DownloadFactory(control->core());

    f->set_session(true);
    f->set_init_load(true);
    f->slot_finished([f](){ delete f; });
    f->load(entries.path() + entry.s_name);
    f->commit();
  }
}

void
load_arg_torrents(char** first, char** last) {
  for (; first != last; ++first) {
    auto* f = new core::DownloadFactory(control->core());

    f->set_start(true);
    f->set_init_load(true);
    f->slot_finished([f](){ delete f; });
    f->load(*first);
    f->commit();
  }
}
