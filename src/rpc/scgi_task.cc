#include "config.h"

#include "rpc/parse_commands.h"
#include <cstdio>
#include <rak/allocators.h>
#include <rak/error_number.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <torrent/exceptions.h>
#include <torrent/poll.h>
#include <torrent/utils/log.h>
#include <vector>
#include <zlib.h>

#include "utils/socket_fd.h"

#include "control.h"
#include "globals.h"
#include "scgi.h"

// Test:
// #include "core/manager.h"
// #include <rak/timer.h>

// static rak::timer scgiTimer;

namespace rpc {

// If bufferSize is zero then memcpy won't do anything.
inline void
SCgiTask::realloc_buffer(uint32_t size, const char* buffer, uint32_t bufferSize) {
  char* tmp = rak::cacheline_allocator<char>::alloc_size(size);

  std::memcpy(tmp, buffer, bufferSize);
  ::free(m_buffer);
  m_buffer = tmp;
}

void
SCgiTask::open(SCgi* parent, int fd) {
  m_parent   = parent;
  m_fileDesc = fd;
  m_buffer   = rak::cacheline_allocator<char>::alloc_size((m_bufferSize = default_buffer_size) + 1);
  m_position = m_buffer;
  m_body     = NULL;

  worker_thread->poll()->open(this);
  worker_thread->poll()->insert_read(this);
  worker_thread->poll()->insert_error(this);

  //   scgiTimer = rak::timer::current();
}

void
SCgiTask::close() {
  if (!get_fd().is_valid())
    return;

  worker_thread->poll()->remove_read(this);
  worker_thread->poll()->remove_write(this);
  worker_thread->poll()->remove_error(this);
  worker_thread->poll()->close(this);

  get_fd().close();
  get_fd().clear();

  ::free(m_buffer);
  m_buffer = NULL;

  // Test
  //   char buffer[512];
  //   sprintf(buffer, "SCgi system call processed: %i", (int)(rak::timer::current() - scgiTimer).usec());
  //   control->core()->push_log(std::string(buffer));
}

void
SCgiTask::event_read() {
  int bytes = ::recv(m_fileDesc, m_position, m_bufferSize - (m_position - m_buffer), 0);

  if (bytes <= 0) {
    if (bytes == 0 || !rak::error_number::current().is_blocked_momentary())
      close();

    return;
  }

  // The buffer has space to nul-terminate to ease the parsing below.
  m_position += bytes;
  *m_position = '\0';

  if (m_body == NULL) {
    // Don't bother caching the parsed values, as we're likely to
    // receive all the data we need the first time.
    char* current;

    int   header_size = strtol(m_buffer, &current, 0);

    if (current == m_position)
      return;

    // If the request doesn't start with an integer or if it didn't
    // end in ':', then close the connection.
    if (current == m_buffer || *current != ':' || header_size < 17 || header_size > max_header_size)
      goto event_read_failed;

    if (std::distance(++current, m_position) < header_size + 1)
      return;

    // We'll parse this fully below, but the SCGI spec requires it to
    // be the first header.
    if (std::memcmp(current, "CONTENT_LENGTH", 15) != 0)
      goto event_read_failed;

    std::string content_type   = "";
    size_t      content_length = 0;
    const char* header_end     = current + header_size;

    // Parse out the null-terminated header keys and values, with
    // checks to ensure it doesn't scan beyond the limits of the
    // header
    while (current < header_end) {
      char* key     = current;
      char* key_end = static_cast<char*>(std::memchr(current, '\0', header_end - current));
      if (!key_end)
        goto event_read_failed;

      current = key_end + 1;
      if (current >= header_end)
        goto event_read_failed;

      char* value     = current;
      char* value_end = static_cast<char*>(std::memchr(current, '\0', header_end - current));
      if (!value_end)
        goto event_read_failed;
      current = value_end + 1;

      if (strcmp(key, "CONTENT_LENGTH") == 0) {
        char* content_pos;
        content_length = strtol(value, &content_pos, 10);
        if (*content_pos != '\0' || content_length <= 0 || content_length > max_content_size)
          goto event_read_failed;
      } else if (strcmp(key, "CONTENT_TYPE") == 0) {
        content_type = value;
      } else if (strcmp(key, "ACCEPT_ENCODING") == 0) {
        if (strstr(value, "gzip") != nullptr)
          // This just marks it as possible to compress, it may not
          // actually happen depending on the size of the response
          m_compress_response = true;
      }
    }

    if (current != header_end)
      goto event_read_failed;

    if (content_length <= 0)
      goto event_read_failed;

    m_body      = current + 1;
    header_size = std::distance(m_buffer, m_body);

    if (content_type == "") {
      // If no CONTENT_TYPE was supplied, peek at the body to check if it's JSON
      // { is a single request object, while [ is a batch array
      if (*m_body == '{' || *m_body == '[')
        m_content_type = ContentType::JSON;
    } else if (content_type == "application/json") {
      m_content_type = ContentType::JSON;
    } else if (content_type == "text/xml") {
      m_content_type = ContentType::XML;
    } else {
      goto event_read_failed;
    }

    if ((unsigned int)(content_length + header_size) < m_bufferSize) {
      m_bufferSize = content_length + header_size;

    } else if ((unsigned int)content_length <= default_buffer_size) {
      m_bufferSize = content_length;

      std::memmove(m_buffer, m_body, std::distance(m_body, m_position));
      m_position = m_buffer + std::distance(m_body, m_position);
      m_body     = m_buffer;

    } else {
      realloc_buffer((m_bufferSize = content_length) + 1, m_body, std::distance(m_body, m_position));

      m_position = m_buffer + std::distance(m_body, m_position);
      m_body     = m_buffer;
    }
  }

  if ((unsigned int)std::distance(m_buffer, m_position) != m_bufferSize)
    return;

  worker_thread->poll()->remove_read(this);
  worker_thread->poll()->insert_write(this);

  if (m_parent->log_fd() >= 0) {
    int __UNUSED result;

    // Clean up logging, this is just plain ugly...
    //    write(m_logFd, "\n---\n", sizeof("\n---\n"));
    result = write(m_parent->log_fd(), m_buffer, m_bufferSize);
    result = write(m_parent->log_fd(), "\n---\n", sizeof("\n---\n"));
  }

  lt_log_print_dump(torrent::LOG_RPC_DUMP, m_body, m_bufferSize - std::distance(m_buffer, m_body), "scgi", "RPC read.", 0);

  // Close if the call failed, else stay open to write back data.
  if (!m_parent->receive_call(this, m_body, m_bufferSize - std::distance(m_buffer, m_body)))
    close();

  return;

event_read_failed:
  //   throw torrent::internal_error("SCgiTask::event_read() fault not handled.");
  close();
}

void
SCgiTask::event_write() {
  int bytes = -1;

// Apple and Solaris do not support MSG_NOSIGNAL,
// so disable this fix until we find a better solution
#if defined(__APPLE__) || defined(__sun__)
  bytes = ::send(m_fileDesc, m_position, m_bufferSize, 0);
#else
  bytes = ::send(m_fileDesc, m_position, m_bufferSize, MSG_NOSIGNAL);
#endif

  if (bytes == -1) {
    if (!rak::error_number::current().is_blocked_momentary())
      close();

    return;
  }

  m_position += bytes;
  m_bufferSize -= bytes;

  if (bytes == 0 || m_bufferSize == 0)
    return close();
}

void
SCgiTask::event_error() {
  close();
}

// Convenience function similar to zlib's compress(), but uses a gzip
// header. Returns an empty string on error, as even a compressed
// empty string will have data.
std::string
gzip_compress(const char* buffer, uint32_t length) {
  std::string compressed;

  // The choice of 1/3 is based on some very rough tests of JSON/XML
  // compression gains, aiming for just enough to skip the initial
  // round of memory allocations
  compressed.reserve(length / 3);
  z_stream zs;
  zs.zalloc = Z_NULL;
  zs.zfree  = Z_NULL;
  zs.opaque = Z_NULL;

  constexpr int window_bits   = 15;
  constexpr int gzip_encoding = 16;
  constexpr int gzip_level    = 6;
  constexpr int chunk_size    = 16384;

  if (deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, window_bits | gzip_encoding, gzip_level, Z_DEFAULT_STRATEGY) != Z_OK)
    return {};

  zs.next_in  = (Bytef*)buffer;
  zs.avail_in = length;
  unsigned char out[chunk_size];
  do {
    zs.avail_out = sizeof(out);
    zs.next_out  = out;
    if (deflate(&zs, Z_FINISH) == Z_STREAM_ERROR)
      return {};
    compressed.append(reinterpret_cast<char*>(out), sizeof(out) - zs.avail_out);
  } while (zs.avail_out == 0);

  return compressed;
}

bool
SCgiTask::receive_write(const char* buffer, uint32_t length) {
  if (buffer == NULL || length > (100 << 20))
    throw torrent::internal_error("SCgiTask::receive_write(...) received bad input.");

  std::string header = m_content_type == ContentType::JSON
                         ? "Status: 200 OK\r\nContent-Type: application/json\r\nContent-Length: %i\r\n"
                         : "Status: 200 OK\r\nContent-Type: text/xml\r\nContent-Length: %i\r\n";

  // Write to log prior to possible compression
  if (m_parent->log_fd() >= 0) {
    int __UNUSED result;
    // Clean up logging, this is just plain ugly...
    //    write(m_logFd, "\n---\n", sizeof("\n---\n"));
    result = write(m_parent->log_fd(), buffer, length);
    result = write(m_parent->log_fd(), "\n---\n", sizeof("\n---\n"));
  }

  lt_log_print_dump(torrent::LOG_RPC_DUMP, buffer, length, "scgi", "RPC write.", 0);

  bool should_compress = false;
  if (m_compress_response) {
    auto min_size = rpc::call_command_value("network.gzip_response_min_size");
    if (min_size >= 0 && length > min_size)
      should_compress = true;
  }

  std::string compressed_buffer;
  if (should_compress) {
    compressed_buffer = gzip_compress(buffer, length);
    if (compressed_buffer != "") {
      header += "Content-Encoding: gzip\r\n";
      length = compressed_buffer.size();
    } else {
      // Fall back to uncompressed response if compression fails
      should_compress = false;
    }
  }
  header += "\r\n";

  int header_size = snprintf(NULL, 0, header.c_str(), length);

  // Need to cast due to a bug in MacOSX gcc-4.0.1.
  if (length + header_size > std::max(m_bufferSize, (unsigned int)default_buffer_size))
    realloc_buffer(length + header_size, NULL, 0);

  m_position   = m_buffer;
  m_bufferSize = length + header_size;

  snprintf(m_buffer, m_bufferSize, header.c_str(), length);
  if (should_compress) {
    std::memcpy(m_buffer + header_size, compressed_buffer.c_str(), length);
  } else {
    std::memcpy(m_buffer + header_size, buffer, length);
  }

  event_write();
  return true;
}

} // namespace rpc
