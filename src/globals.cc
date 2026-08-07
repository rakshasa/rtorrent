#include "config.h"

#include "globals.h"

#include <cstdlib>
#include <stdlib.h>
#include <torrent/exceptions.h>

rpc::ip_table_list ip_tables;

Control*           control{};

std::string
expand_path(const std::string& path) {
  if (path.empty())
    return std::string();

  if (path[0] == '~') {
    if (path.size() >= 2 && path[1] != '/')
      throw torrent::input_error("Could not expand ~ in path, only '~' or '~/...' is supported.");

    const char* home = std::getenv("HOME");

    if (home == nullptr || *home == '\0')
      throw torrent::input_error("Could not expand ~ in path, HOME environment variable not set.");

    return home + path.substr(1);
  }

  return path;
}

// Resolves a path to a canonical one with no symlinks or relative components,
// so it can safely be handed to an external script. Returns an empty string if
// the path does not name an existing file or directory.
std::string
resolve_path(const std::string& path) {
  if (path.empty())
    return std::string();

  char* resolved = ::realpath(expand_path(path).c_str(), nullptr);

  if (resolved == nullptr)
    return std::string();

  std::string result(resolved);
  std::free(resolved);

  return result;
}

std::string
resolve_path_or_throw(const std::string& path) {
  auto result = resolve_path(path);

  if (result.empty())
    throw torrent::input_error("Could not resolve path: '" + path + "'.");

  return result;
}
