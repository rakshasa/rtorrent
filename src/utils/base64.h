#ifndef RTORRENT_UTILS_BASE64_H
#define RTORRENT_UTILS_BASE64_H

#include <optional>
#include <string>
#include <vector>
#include <torrent/exceptions.h>

namespace utils {

// TODO: Refactor and move to torrent/string_manip.h.
std::optional<std::vector<uint8_t>> base64_to_vector_unsafe(const std::string& src);
std::string                         openssl_base64_encode(const std::string& src);

// TODO: Deprecate.
std::string remove_newlines(const std::string& str);
std::string decode_base64(const std::string& input);

} // namespace utils

#endif
