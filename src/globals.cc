#include "config.h"

#include "globals.h"

#include <torrent/exceptions.h>

rpc::ip_table_list ip_tables;

Control*           control{};

std::string
expand_path(const std::string& path) {
  if (path.empty())
    return std::string();

  if (path[0] == '~') {
    if (path.size() < 2 || path[1] != '/')
      throw torrent::input_error("Could not expand ~ in session path, only ~/<path> is supported.");

    const char* home = std::getenv("HOME");

    if (home == nullptr || *home == '\0')
      throw torrent::input_error("Could not expand ~ in session path, HOME environment variable not set.");

    return home + path.substr(1);
  }

  return path;
}
