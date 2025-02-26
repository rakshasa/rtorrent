#include "config.h"

#include <cstdio>
#include <cstdlib>
#include <rak/allocators.h>
#include <rak/error_number.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <torrent/exceptions.h>
#include <torrent/poll.h>
#include <torrent/utils/log.h>

#include "utils/socket_fd.h"
#include "utils/gzip.h"

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

void
SCgiTask::open(SCgi* parent, int fd) {
  m_buffer.resize(default_buffer_size + 1);

  m_parent    = parent;
  m_fileDesc  = fd;
  m_position  = m_buffer.data();
  m_body      = nullptr;

  worker_thread->poll()->open(this);
  worker_thread->poll()->insert_read(this);
  worker_thread->poll()->insert_error(this);
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

  m_position  = nullptr;
  m_body     = nullptr;

  // Test
  //   char buffer[512];
  //   sprintf(buffer, "SCgi system call processed: %i", (int)(rak::timer::current() - scgiTimer).usec());
  //   control->core()->push_log(std::string(buffer));
}

void
SCgiTask::event_read() {
  int bytes = ::recv(m_fileDesc, m_position, m_buffer.size() - (m_position - m_buffer.data()), 0);

  if (bytes <= 0) {
    if (bytes == 0 || !rak::error_number::current().is_blocked_momentary())
      close();

    return;
  }

  // The buffer has space to nul-terminate to ease the parsing below.
  m_position += bytes;
  *m_position = '\0';

  if (m_body == NULL) {
    switch (read_header()) {
    case 0:
      break;
    case 1:
      return;
    default:
      close();
      return;
    };
  }

  if ((size_t)std::distance(m_buffer.data(), m_position) != m_buffer.size())
    return;

  worker_thread->poll()->remove_read(this);
  worker_thread->poll()->insert_write(this);

  if (m_parent->log_fd() >= 0) {
    int __UNUSED result;

    // Clean up logging, this is just plain ugly...
    //    write(m_logFd, "\n---\n", sizeof("\n---\n"));
    result = write(m_parent->log_fd(), m_buffer.data(), m_buffer.size());
    result = write(m_parent->log_fd(), "\n---\n", sizeof("\n---\n"));
  }

  auto body_size = m_buffer.size() - std::distance(m_buffer.data(), m_body);

  lt_log_print_dump(torrent::LOG_RPC_DUMP, m_body, body_size, "scgi", "RPC read.", 0);

  // Close if the call failed, else stay open to write back data.
  if (!m_parent->receive_call(this, m_body, body_size))
    close();
}

// 0 - Success
// 1 - Not enough data
// 2 - Invalid header
int
SCgiTask::read_header() {
  // Don't bother caching the parsed values, as we're likely to
  // receive all the data we need the first time.
  char* current;
  int   header_size = std::strtol(m_buffer.data(), &current, 0);

  if (current == m_position)
    return 1;

  // If the request doesn't start with an integer or if it didn't
  // end in ':', then close the connection.
  if (current == m_buffer.data() || *current != ':' || header_size < 17 || header_size > max_header_size)
    return 2;

  if (std::distance(++current, m_position) < header_size + 1)
    return 1;

  // We'll parse this fully below, but the SCGI spec requires it to
  // be the first header.
  if (std::memcmp(current, "CONTENT_LENGTH", 15) != 0)
    return 2;

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
      return 2;

    current = key_end + 1;

    if (current >= header_end)
      return 2;

    char* value     = current;
    char* value_end = static_cast<char*>(std::memchr(current, '\0', header_end - current));

    if (!value_end)
      return 2;

    current = value_end + 1;

    if (strcmp(key, "CONTENT_LENGTH") == 0) {
      char* content_pos;
      content_length = strtol(value, &content_pos, 10);

      if (*content_pos != '\0' || content_length <= 0 || content_length > max_content_size)
        return 2;

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
    return 2;

  if (content_length <= 0)
    return 2;

  m_body      = current + 1;
  header_size = std::distance(m_buffer.data(), m_body);

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
    return 2;
  }

  auto position_index = std::distance(m_buffer.data(), m_position);

  m_buffer.resize(content_length + header_size);

  m_position = m_buffer.data() + position_index;
  m_body = m_buffer.data() + header_size;

  return 0;
}

void
SCgiTask::event_write() {
  int bytes;
  size_t remaining = m_buffer.size() - std::distance(m_buffer.data(), m_position);

// Apple and Solaris do not support MSG_NOSIGNAL,
// so disable this fix until we find a better solution
#if defined(__APPLE__) || defined(__sun__)
  bytes = ::send(m_fileDesc, m_position, remaining, 0);
#else
  bytes = ::send(m_fileDesc, m_position, remaining, MSG_NOSIGNAL);
#endif

  if (bytes == -1) {
    if (!rak::error_number::current().is_blocked_momentary())
      close();

    return;
  }

  m_position += bytes;

  if (bytes == 0 || remaining == 0)
    return close();
}

void
SCgiTask::event_error() {
  close();
}

// TODO: Pass ownership of the buffer to ScgiTask to reduce copying for plaintext responses.
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
  if (m_client_accepts_compressed_response && gzip_enabled() && (int)length > gzip_min_size())
    gzip_compress_response(buffer, length);
  else
    plaintext_response(buffer, length);

  event_write();
  return true;
}

void
SCgiTask::gzip_compress_response(const char* buffer, uint32_t length) {
  static const std::string header_first = "Status: 200 OK\r\nContent-Type: application/json\r\nContent-Length: ";
  static const std::string header_last  = "\r\nContent-Encoding: gzip\r\n\r\n";

  utils::gzip_compress_to_vector(buffer, length, m_buffer, max_header_size);

  auto content_length = std::to_string(m_buffer.size() - max_header_size);
  auto start_position = m_buffer.data() + max_header_size - (header_first.size() + content_length.size() + header_last.size());

  std::memcpy(start_position, header_first.data(), header_first.size());
  std::memcpy(start_position + header_first.size(), content_length.data(), content_length.size());
  std::memcpy(start_position + header_first.size() + content_length.size(), header_last.data(), header_last.size());
}

void
SCgiTask::plaintext_response(const char* buffer, uint32_t length) {
  static const std::string header_first = "Status: 200 OK\r\nContent-Type: text/plain\r\nContent-Length: ";
  static const std::string header_last  = "\r\n\r\n";

  auto content_length = std::to_string(length);

  m_buffer.resize(header_first.size() + content_length.size() + header_last.size() + length);

  std::memcpy(m_buffer.data(), header_first.data(), header_first.size());
  std::memcpy(m_buffer.data() + header_first.size(), content_length.data(), content_length.size());
  std::memcpy(m_buffer.data() + header_first.size() + content_length.size(), header_last.data(), header_last.size());
  std::memcpy(m_buffer.data() + header_first.size() + content_length.size() + header_last.size(), buffer, length);

  m_position = m_buffer.data();
}

} // namespace rpc
