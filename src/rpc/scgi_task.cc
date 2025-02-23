#include "config.h"

#include <cstdio>
#include <rak/allocators.h>
#include <rak/error_number.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <torrent/exceptions.h>
#include <torrent/poll.h>
#include <torrent/utils/log.h>
#include <zlib.h>

#include "utils/socket_fd.h"

#include "scgi_task.h"
#include "control.h"
#include "globals.h"
#include "scgi.h"

// Test:
// #include "core/manager.h"
// #include <rak/timer.h>

// static rak::timer scgiTimer;

namespace rpc {

// Disable gzipping by default, but once enabled gzip everything
bool SCgiTask::m_allow_compressed_response  = false;
int  SCgiTask::m_min_compress_response_size = 0;

// If bufferSize is zero then memcpy won't do anything.
inline void
SCgiTask::realloc_buffer(uint32_t size, const char* buffer, uint32_t buffer_size) {
  char* tmp = rak::cacheline_allocator<char>::alloc_size(size);

  std::memcpy(tmp, buffer, buffer_size);
  ::free(m_buffer);
  m_buffer = tmp;
}

void
SCgiTask::open(SCgi* parent, int fd) {
  m_parent   = parent;
  m_fileDesc = fd;
  m_buffer   = rak::cacheline_allocator<char>::alloc_size((m_buffer_size = default_buffer_size) + 1);
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
  int bytes = ::recv(m_fileDesc, m_position, m_buffer_size - (m_position - m_buffer), 0);

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
          m_client_accepts_compressed_response = true;
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

    if ((unsigned int)(content_length + header_size) < m_buffer_size) {
      m_buffer_size = content_length + header_size;

    } else if ((unsigned int)content_length <= default_buffer_size) {
      m_buffer_size = content_length;

      std::memmove(m_buffer, m_body, std::distance(m_body, m_position));
      m_position = m_buffer + std::distance(m_body, m_position);
      m_body     = m_buffer;

    } else {
      realloc_buffer((m_buffer_size = content_length) + 1, m_body, std::distance(m_body, m_position));

      m_position = m_buffer + std::distance(m_body, m_position);
      m_body     = m_buffer;
    }
  }

  if ((unsigned int)std::distance(m_buffer, m_position) != m_buffer_size)
    return;

  worker_thread->poll()->remove_read(this);
  worker_thread->poll()->insert_write(this);

  if (m_parent->log_fd() >= 0) {
    int __UNUSED result;

    // Clean up logging, this is just plain ugly...
    //    write(m_logFd, "\n---\n", sizeof("\n---\n"));
    result = write(m_parent->log_fd(), m_buffer, m_buffer_size);
    result = write(m_parent->log_fd(), "\n---\n", sizeof("\n---\n"));
  }

  lt_log_print_dump(torrent::LOG_RPC_DUMP, m_body, m_buffer_size - std::distance(m_buffer, m_body), "scgi", "RPC read.", 0);

  // Close if the call failed, else stay open to write back data.
  if (!m_parent->receive_call(this, m_body, m_buffer_size - std::distance(m_buffer, m_body)))
    close();

  return;

event_read_failed:
  //   throw torrent::internal_error("SCgiTask::event_read() fault not handled.");
  close();
}

void
SCgiTask::event_write() {
  int bytes;
// Apple and Solaris do not support MSG_NOSIGNAL,
// so disable this fix until we find a better solution
#if defined(__APPLE__) || defined(__sun__)
  bytes = ::send(m_fileDesc, m_position, m_bufferSize, 0);
#else
  bytes = ::send(m_fileDesc, m_position, m_buffer_size, MSG_NOSIGNAL);
#endif

  if (bytes == -1) {
    if (!rak::error_number::current().is_blocked_momentary())
      close();

    return;
  }

  m_position += bytes;
  m_buffer_size -= bytes;

  if (bytes == 0 || m_buffer_size == 0)
    return close();
}

void
SCgiTask::event_error() {
  close();
}

// On failure, returns false and the buffer is left in an
// indeterminate state, but m_position and m_bufferSize remain the
// same.
bool
SCgiTask::gzip_compress_response(const char* buffer, uint32_t length, std::string_view header_template) {
  z_stream zs;
  zs.zalloc = Z_NULL;
  zs.zfree  = Z_NULL;
  zs.opaque = Z_NULL;

  constexpr int window_bits   = 15;
  constexpr int gzip_encoding = 16;
  constexpr int gzip_level    = 6;
  constexpr int chunk_size    = 16384;

  if (deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, window_bits | gzip_encoding, gzip_level, Z_DEFAULT_STRATEGY) != Z_OK)
    return false;

  // Calculate the maximum size the buffer could reach, note that the
  // max repsonse size will usually be larger than the original length
  const auto max_response_size = deflateBound(&zs, length);
  if (max_response_size + max_header_size > std::max(m_buffer_size, (unsigned int)default_buffer_size))
    realloc_buffer(max_response_size + max_header_size, NULL, 0);

  auto output = m_buffer + max_header_size;
  zs.next_in  = (Bytef*)buffer;
  zs.avail_in = length;
  do {
    zs.avail_out = chunk_size;
    zs.next_out  = (Bytef*)output;
    if (deflate(&zs, Z_FINISH) == Z_STREAM_ERROR)
      return false;
    output += chunk_size - zs.avail_out;
  } while (zs.avail_out == 0);

  // Write the header directly to the buffer. If at any point
  // max_header_size would be exceeded, fail gracefully.
  const std::string_view header_end("Content-Encoding: gzip\r\n\r\n");
  const int              response_size = output - (m_buffer + max_header_size);
  int                    header_size   = snprintf(m_buffer, max_header_size, header_template.data(), response_size);
  if (header_size < 0 || header_end.size() > max_header_size - header_size)
    return false;
  std::memcpy(m_buffer + header_size, header_end.data(), header_end.size());
  header_size += header_end.size();

  // Move the response back into position right after the headers
  std::memmove(m_buffer + header_size, m_buffer + max_header_size, response_size);

  m_position   = m_buffer;
  m_buffer_size = header_size + response_size;
  return true;
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

  // Compress the response if possible
  if (m_client_accepts_compressed_response &&
      gzip_enabled() &&
      length > gzip_min_size() &&
      gzip_compress_response(buffer, length, header)) {
    event_write();
    return true;
  }

  // Otherwise (or if the compression fails), just copy the bytes
  header += "\r\n";

  int header_size = snprintf(NULL, 0, header.c_str(), length);

  // Need to cast due to a bug in MacOSX gcc-4.0.1.
  if (length + header_size > std::max(m_buffer_size, (unsigned int)default_buffer_size))
    realloc_buffer(length + header_size, NULL, 0);

  m_position   = m_buffer;
  m_buffer_size = length + header_size;

  snprintf(m_buffer, m_buffer_size, header.c_str(), length);
  std::memcpy(m_buffer + header_size, buffer, length);

  event_write();
  return true;
}

} // namespace rpc
