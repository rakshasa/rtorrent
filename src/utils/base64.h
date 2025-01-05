#ifndef RTORRENT_UTILS_BASE64_H
#define RTORRENT_UTILS_BASE64_H

#include <string>

#include <torrent/exceptions.h>

namespace utils {

std::string remove_newlines(const std::string& str);
std::string decode_base64(const std::string& input);

} // namespace utils

#endif
