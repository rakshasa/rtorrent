#include "config.h"

#include "rpc/scgi_task.h"

#include <rak/error_number.h>
#include <cstdio>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <torrent/exceptions.h>
#include <torrent/torrent.h>
#include <torrent/net/poll.h>
#include <torrent/utils/log.h>
#include <torrent/utils/thread.h>

#include "control.h"
#include "globals.h"
#include "scgi.h"
#include "rpc/parse_commands.h"
#include "utils/socket_fd.h"

namespace rpc {

void
SCgiTask::open(SCgi* parent, int fd) {
  m_parent      = parent;
  m_fileDesc    = fd;
  m_buffer      = new char[default_buffer_size + 1];
  m_buffer_size = default_buffer_size;
  m_position    = m_buffer;
  m_body        = NULL;

  torrent::this_thread::poll()->open(this);
  torrent::this_thread::poll()->insert_read(this);
  torrent::this_thread::poll()->insert_error(this);
}

void
SCgiTask::close() {
  if (!get_fd().is_valid())
    return;

  torrent::main_thread::thread()->cancel_callback_and_wait(this);
  torrent::utils::Thread::self()->cancel_callback(this);

  torrent::this_thread::poll()->remove_and_close(this);

  get_fd().close();
  get_fd().clear();

  auto lock = std::lock_guard<std::mutex>(m_result_mutex);

  delete[] m_buffer;
  m_buffer = NULL;
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
      }
    }

    if (current != header_end)
      goto event_read_failed;

    if (content_length <= 0)
      goto event_read_failed;

    m_body      = current + 1;
    header_size = std::distance(m_buffer, m_body);

    if (!detect_content_type(content_type))
      goto event_read_failed;

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

  torrent::this_thread::poll()->remove_read(this);

  if (m_parent->log_fd() >= 0) {
    [[maybe_unused]] int result;

    // Clean up logging, this is just plain ugly...
    //    write(m_logFd, "\n---\n", sizeof("\n---\n"));
    result = write(m_parent->log_fd(), m_buffer, m_buffer_size);
    result = write(m_parent->log_fd(), "\n---\n", sizeof("\n---\n"));
  }

  lt_log_print_dump(torrent::LOG_RPC_DUMP, m_body, m_buffer_size - std::distance(m_buffer, m_body), "scgi", "RPC read.", 0);

  receive_call(m_body, m_buffer_size - std::distance(m_buffer, m_body));
  return;

event_read_failed:
  //   throw torrent::internal_error("SCgiTask::event_read() fault not handled.");
  close();
}

void
SCgiTask::event_write() {
// Apple and Solaris do not support MSG_NOSIGNAL,
// so disable this fix until we find a better solution
#if defined(__APPLE__) || defined(__sun__)
  int bytes = ::send(m_fileDesc, m_position, m_buffer_size, 0);
#else
  int bytes = ::send(m_fileDesc, m_position, m_buffer_size, MSG_NOSIGNAL);
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

static inline bool
scgi_match_content_type(const std::string& content_type, const char* type) {
  std::string::size_type pos = content_type.find_first_of(" ;");

  if (pos == std::string::npos)
    return content_type == type;

  return content_type.compare(0, pos, type) == 0;
}

bool
SCgiTask::detect_content_type(const std::string& content_type) {
  if (content_type.empty()) {
    // If no CONTENT_TYPE was supplied, peek at the body to check if it's JSON
    // { is a single request object, while [ is a batch array
    if (*m_body == '{' || *m_body == '[')
      m_content_type = ContentType::JSON;
    else
      m_content_type = ContentType::XML;

  } else if (scgi_match_content_type(content_type, "application/json")) {
    m_content_type = ContentType::JSON;

  } else if (scgi_match_content_type(content_type, "text/xml")) {
    m_content_type = ContentType::XML;

  } else {
    // If the content type is not JSON or XML, we don't know how to handle it.
    return false;
  }

  return true;
}

// If bufferSize is zero then memcpy won't do anything.
void
SCgiTask::realloc_buffer(uint32_t size, const char* buffer, uint32_t bufferSize) {
  char* tmp = new char[size];

  std::memcpy(tmp, buffer, bufferSize);
  ::free(m_buffer);
  m_buffer = tmp;
}

void
SCgiTask::receive_call(const char* buffer, uint32_t length) {
  // TODO: Rewrite RpcManager.process to pass the result buffer instead of having to copy it.

  auto scgi_thread = torrent::utils::Thread::self();

  auto result_callback = [this, scgi_thread](const char* b, uint32_t l) {
      receive_write(b, l);

      scgi_thread->callback_interrupt_pollling(this, [this]() {
          // Only need to lock once here as a memory barrier.
          m_result_mutex.lock();
          m_result_mutex.unlock();

          torrent::this_thread::poll()->insert_write(this);
        });
    };

  auto lock = std::lock_guard<std::mutex>(m_result_mutex);

  switch (content_type()) {
  case rpc::SCgiTask::ContentType::JSON:
    torrent::main_thread::thread()->callback_interrupt_pollling(this, [buffer, length, result_callback]() {
        rpc.process(RpcManager::RPCType::JSON, buffer, length,
                    [result_callback](const char* b, uint32_t l) {
                      result_callback(b, l);
                      return true;
                    });
      });
    break;

  case rpc::SCgiTask::ContentType::XML:
    torrent::main_thread::thread()->callback_interrupt_pollling(this, [buffer, length, result_callback]() {
        rpc.process(RpcManager::RPCType::XML, buffer, length,
                    [result_callback](const char* b, uint32_t l) {
                      result_callback(b, l);
                      return true;
                    });
      });
    break;

  default:
    throw torrent::internal_error("SCgiTask::receive_call(...) received bad input.");
  }
}

void
SCgiTask::receive_write(const char* buffer, uint32_t length) {
  if (buffer == NULL || length > (100 << 20))
    throw torrent::internal_error("SCgiTask::receive_write(...) received bad input.");

  auto lock = std::lock_guard<std::mutex>(m_result_mutex);

  // Need to cast due to a bug in MacOSX gcc-4.0.1.
  if (length + 256 > std::max(m_buffer_size, (unsigned int)default_buffer_size))
    realloc_buffer(length + 256, NULL, 0);

  const auto header = m_content_type == ContentType::JSON
                        ? "Status: 200 OK\r\nContent-Type: application/json\r\nContent-Length: %i\r\n\r\n"
                        : "Status: 200 OK\r\nContent-Type: text/xml\r\nContent-Length: %i\r\n\r\n";

  // Who ever bothers to check the return value?
  int headerSize = snprintf(m_buffer, m_buffer_size, header, length);

  m_position   = m_buffer;
  m_buffer_size = length + headerSize;

  std::memcpy(m_buffer + headerSize, buffer, length);

  if (m_parent->log_fd() >= 0) {
    [[maybe_unused]] int result;
    result = write(m_parent->log_fd(), m_buffer, m_buffer_size);
    result = write(m_parent->log_fd(), "\n---\n", sizeof("\n---\n"));
  }

  lt_log_print_dump(torrent::LOG_RPC_DUMP, m_buffer, m_buffer_size, "scgi", "RPC write.", 0);
}

} // namespace rpc
