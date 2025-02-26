#include "config.h"

#include "utils/gzip.h"

#include <torrent/exceptions.h>
#include <zlib.h>

namespace utils {

void
gzip_compress_to_vector(const char* buffer, unsigned int length, std::vector<char>& output, unsigned int offset) {
  z_stream zs;
  zs.zalloc = Z_NULL;
  zs.zfree  = Z_NULL;
  zs.opaque = Z_NULL;

  constexpr int window_bits   = 15;
  constexpr int gzip_encoding = 16;
  constexpr int gzip_level    = 6;

  if (deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, window_bits | gzip_encoding, gzip_level, Z_DEFAULT_STRATEGY) != Z_OK)
    throw torrent::internal_error("gzip_compress_to_vector(...) could not initialize gzip deflate.");

  auto max_response_size = deflateBound(&zs, length);

  output.resize(offset + max_response_size);

  zs.next_in  = (Bytef*)buffer;
  zs.avail_in = length;
  zs.next_out  = (Bytef*)(output.data() + offset);
  zs.avail_out = max_response_size;

  int ret = deflate(&zs, Z_FINISH);

  if (ret != Z_STREAM_END)
    throw torrent::internal_error("gzip_compress_to_vector(...) deflate did not return Z_STREAM_END: " + std::to_string(ret));

  output.resize(offset + max_response_size - zs.avail_out);
}

} // namespace utils
