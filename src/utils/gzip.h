#ifndef RTORRENT_UTILS_GZIP_H
#define RTORRENT_UTILS_GZIP_H

#include <functional>

namespace utils {

void gzip_compress_to_vector(const char* buffer, unsigned int length, std::vector<char>& output, unsigned int offset = 0);

} // namespace utils

#endif
