#ifndef RTORRENT_SETUP_H
#define RTORRENT_SETUP_H

#include <functional>
#include <string>

int  parse_main_options(int argc, char** argv);
void parse_config_file(int argc, char** argv, std::function<void (const std::string&)> parse_fn);
void parse_config_file_comments(const std::string& path);

void load_session_torrents(const std::string& path);
void load_arg_torrents(char** first, char** last);

static constexpr int log_flag_use_gz      = 0x1;
static constexpr int log_flag_append_pid  = 0x2;
static constexpr int log_flag_append_file = 0x4;
static constexpr int log_flag_flush       = 0x8;

void log_add_group_output_str(const std::string& group_name, const std::string& output_id);
void apply_log_open_str(int output_flags, const std::vector<std::string>& args);

#endif
