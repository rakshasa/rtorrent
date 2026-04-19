#ifndef RTORRENT_SETUP_H
#define RTORRENT_SETUP_H

#include <functional>
#include <string>

int  parse_main_options(int argc, char** argv);
void parse_config_file(int argc, char** argv, std::function<void (const std::string&)> parse_fn);

void load_session_torrents(const std::string& path);
void load_arg_torrents(char** first, char** last);

#endif
