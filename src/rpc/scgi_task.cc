#include "config.h"

#include "rpc/scgi_task.h"

#include <atomic>
#include <cassert>
#include <charconv>
#include <cstdio>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <torrent/exceptions.h>
#include <torrent/net/fd.h>
#include <torrent/net/poll.h>
#include <torrent/runtime/socket_manager.h>
#include <torrent/utils/log.h>
#include <torrent/system/thread.h>

#include "control.h"
#include "globals.h"
#include "rpc/parse_commands.h"
#include "rpc/scgi.h"
#include "utils/gzip.h"

namespace rpc {

void
SCgiTask::open(SCgi* parent, int fd) {
  set_file_descriptor(fd);

  m_parent      = parent;
  m_position    = 0;
  m_body        = 0;

  m_content_length      = 0;
  m_content_type        = XML;
  m_accepts_compression = false;

  torrent::this_thread::poll()->open(this);
  torrent::this_thread::poll()->insert_read(this);
  torrent::this_thread::poll()->insert_error(this);

  auto lock = std::lock_guard<std::mutex>(m_result_mutex);

  // Leave room for terminating nul byte for parsing the header.
  m_buffer.resize(default_buffer_size + 1);
}

void
SCgiTask::cancel_open() {
  if (!is_open())
    return;

  torrent::this_thread::poll()->remove_and_close(this);

  torrent::fd_close(file_descriptor());
  set_file_descriptor(-1);
};

void
SCgiTask::close() {
  if (!is_open())
    return;

  torrent::main_thread::thread()->cancel_callback_and_wait(this);
  torrent::system::Thread::self()->cancel_callback(this);

  torrent::runtime::socket_manager()->close_event_or_throw(this, [this]() {
      torrent::this_thread::poll()->remove_and_close(this);

      torrent::fd_close(file_descriptor());
      set_file_descriptor(-1);
    });

  // The callbacks are guaranteed to be finished/canceled at this point.
  auto lock = std::lock_guard<std::mutex>(m_result_mutex);

  m_buffer.clear();
}

void
SCgiTask::event_read() {
  int read_length = m_buffer.size() - m_position;
  
  if (m_content_length == 0)
    read_length--;
  
  int bytes = ::recv(m_fileDesc, m_buffer.data() + m_position, read_length, 0);

  if (bytes <= 0) {
    if (bytes == 0 || !(errno == EAGAIN || errno == EINTR))
      close();

    return;
  }

  // The buffer has space to nul-terminate to ease the parsing below.
  m_position += bytes;

  if (m_content_length == 0) {
    // While reading the header, we leave room in the buffer for a nul byte.
    m_buffer[m_position] = '\0';

    // Don't bother caching the parsed values, as we're likely to receive all the data we need the
    // first time.
    unsigned int header_size{};

    auto [current, ec] = std::from_chars(m_buffer.data(), m_buffer.data() + m_position, header_size);

    // If the request doesn't start with an integer or if it didn't end in ':', then close the
    // connection.

    if (current == m_buffer.data())
      return;

    if (*current != ':' || header_size < 17 || header_size > max_header_size)
      goto event_read_failed;

    current++;

    // The header size starts after the ':' and ends at the first ','.
    if ((m_buffer.data() + m_position) - current < static_cast<int>(header_size) + 1)
      return;

    // Check for ',' after the header, if it's not there, then close the connection.
    if (*(current + header_size) != ',')
      goto event_read_failed;

    // The header must start with "CONTENT_LENGTH" followed by a null byte.
    if (std::memcmp(current, "CONTENT_LENGTH", 14+1) != 0)
      goto event_read_failed;

    if (!parse_headers(current, header_size))
      goto event_read_failed;
  }

  if (m_position < m_body + m_content_length)
    return;

  torrent::this_thread::poll()->remove_read(this);

  if (m_parent->log_fd() >= 0) {
    [[maybe_unused]] int result;

    // Clean up logging, this is just plain ugly...
    //    write(m_logFd, "\n---\n", sizeof("\n---\n"));
    result = ::write(m_parent->log_fd(), m_buffer.data() + m_body, m_position - m_body);
    result = ::write(m_parent->log_fd(), "\n---\n", sizeof("\n---\n"));
  }

  lt_log_print_dump(torrent::LOG_RPC_DUMP, m_buffer.data() + m_body, m_content_length, "scgi", "RPC read.", 0);

  receive_call(m_buffer.data() + m_body, m_content_length);
  return;

event_read_failed:
  //   throw torrent::internal_error("SCgiTask::event_read() fault not handled.");
  close();
}

void
SCgiTask::event_write() {
  int bytes = ::send(m_fileDesc, m_buffer.data() + m_position, m_buffer.size() - m_position, 0);

  if (bytes == -1) {
    if (!(errno == EAGAIN || errno == EINTR || errno == EPIPE))
      close();

    return;
  }

  m_position += bytes;

  if (bytes == 0 || m_position == m_buffer.size())
    return close();
}

void
SCgiTask::event_error() {
  close();
}

bool
SCgiTask::parse_headers(const char* current, unsigned int header_length) {
  std::string content_type;

  const char* header_end  = current + header_length;

  // Parse out the null-terminated header keys and values, with
  // checks to ensure it doesn't scan beyond the limits of the
  // header
  while (current < header_end) {
    auto* key     = current;
    auto* key_end = static_cast<const char*>(std::memchr(current, '\0', header_end - current));

    if (key_end == nullptr)
      return false;

    current = key_end + 1;

    if (current >= header_end)
      return false;

    auto* value     = current;
    auto* value_end = static_cast<const char*>(std::memchr(current, '\0', header_end - current));

    if (value_end == nullptr)
      return false;

    current = value_end + 1;

    if (std::strncmp(key, "CONTENT_LENGTH", 14+1) == 0) {
      auto [content_pos, ec] = std::from_chars(value, value_end, m_content_length);

      if (*content_pos != '\0' || m_content_length <= 0 || m_content_length > max_content_size)
        return false;

    } else if (std::strncmp(key, "CONTENT_TYPE", 12+1) == 0) {
      content_type = value;

    } else if (std::strncmp(key, "ACCEPT_ENCODING", 15+1) == 0) {
      std::string accept_encoding(value, value_end - value);

      if (accept_encoding.find("gzip") != std::string::npos)
        m_accepts_compression = true;

    } else if (std::strncmp(key, "UNTRUSTED_CONNECTION", 20+1) == 0) {
      if (std::strncmp(value, "1", 1+1) == 0)
        m_trusted = false;
      else if (std::strncmp(value, "0", 1+1) == 0)
        ; // Default is trusted, so do nothing.
      else
        return false;
    }
  }

  if (current != header_end)
    return false;

  if (m_content_length == 0)
    return false;

  // Move past the ',' that ends the header.
  current++;

  if (current > m_buffer.data() + m_buffer.size())
    throw torrent::internal_error("SCgiTask::event_read() header parsing overflow : body start is beyond buffer end");

  m_body = current - m_buffer.data();

  if (!detect_content_type(content_type))
    return false;

  if (m_body + m_content_length > m_buffer.size()) {
    if (m_content_length > default_buffer_size) {
      std::vector<char> tmp(m_content_length);

      std::memcpy(tmp.data(), m_buffer.data() + m_body, m_position - m_body);
      m_buffer.swap(tmp);

    } else {
      std::memmove(m_buffer.data(), m_buffer.data() + m_body, m_position - m_body);
    }

    m_position = m_position - m_body;
    m_body     = 0;
  }

  return true;
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

    if (m_buffer[m_body] == '{' || m_buffer[m_body] == '[')
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

void
SCgiTask::receive_call(const char* buffer, uint32_t length) {
  assert(torrent::system::Thread::self() == scgi_thread::thread());

  RpcManager::RPCType rpc_type;

  switch (content_type()) {
  case rpc::SCgiTask::ContentType::JSON:
    rpc_type = RpcManager::RPCType::JSON;
    break;
  case rpc::SCgiTask::ContentType::XML:
    rpc_type = RpcManager::RPCType::XML;
    break;
  default:
    throw torrent::internal_error("SCgiTask::receive_call(...) received bad input.");
  }

  // TODO: Rewrite RpcManager.process to pass the result buffer instead of having to copy it.
  // TODO: Also allow us to request RpcManager.process to reserve space for a header.
  // TODO: Completely remove the mutex, and align m_buffer?

  auto result_callback = [this](const char* b, uint32_t l) {
      receive_write(b, l);

      // Memory barrier for the result data.
      // std::atomic_thread_fence(std::memory_order_release);
      m_result_mutex.lock();
      m_result_mutex.unlock();

      scgi_thread::thread()->callback_interrupt_polling(this, [this]() {
          if (!is_open())
            return;

          torrent::this_thread::poll()->insert_write(this);

          // Memory barrier for the result data.
          // std::atomic_thread_fence(std::memory_order_acquire);
          m_result_mutex.lock();
          m_result_mutex.unlock();
        });

      return true;
    };

  // Memory barrier for the input data.
  // std::atomic_thread_fence(std::memory_order_release);
  m_result_mutex.lock();
  m_result_mutex.unlock();

  torrent::main_thread::thread()->callback_interrupt_polling(this, [this, rpc_type, buffer, length, result_callback]() {
      // Memory barrier for the input data.
      // std::atomic_thread_fence(std::memory_order_acquire);
      m_result_mutex.lock();
      m_result_mutex.unlock();

      if (m_trusted)
        rpc.process(rpc_type, buffer, length, result_callback);
      else
        rpc.process_untrusted(rpc_type, buffer, length, result_callback);
    });
}

void
SCgiTask::receive_write(const char* buffer, uint32_t length) {
  assert(torrent::system::Thread::self() == torrent::main_thread::thread());

  if (buffer == nullptr || length > (100 << 20))
    throw torrent::internal_error("SCgiTask::receive_write(...) received bad input.");

  // Main thread callback already locked this mutex.
  // auto lock = std::lock_guard<std::mutex>(m_result_mutex);

  // Write to log prior to possible compression
  if (m_parent->log_fd() >= 0) {
    int result [[maybe_unused]];
    // Clean up logging, this is just plain ugly...
    //    write(m_logFd, "\n---\n", sizeof("\n---\n"));
    result = write(m_parent->log_fd(), buffer, length);
    result = write(m_parent->log_fd(), "\n---\n", sizeof("\n---\n"));
  }

  lt_log_print_dump(torrent::LOG_RPC_DUMP, buffer, length, "scgi", "RPC write.", 0);

  if (m_accepts_compression && rpc.scgi_allow_compression() && length > rpc.scgi_min_compress_size())
    gzip_response(buffer, length);
  else
    plaintext_response(buffer, length);
}

// We don't know the size of the content-length string, so leave sufficient space for the header
// strings plus max length of content-length.

void
SCgiTask::plaintext_response(const char* buffer, uint32_t content_length) {
  auto header_first      = content_type() == ContentType::XML ? header_xml : header_json;
  auto header_first_size = content_type() == ContentType::XML ? header_xml_size : header_json_size;
  auto length_str        = std::to_string(content_length);

  m_buffer.resize(header_first_size + length_str.size() + header_last_size + content_length);

  std::memcpy(m_buffer.data(),                                                            header_first, header_first_size);
  std::memcpy(m_buffer.data() + header_first_size,                                        length_str.data(), length_str.size());
  std::memcpy(m_buffer.data() + header_first_size + length_str.size(),                    header_last, header_last_size);
  std::memcpy(m_buffer.data() + header_first_size + length_str.size() + header_last_size, buffer, content_length);

  m_position = 0;
}

void
SCgiTask::gzip_response(const char* buffer, uint32_t content_length) {
  auto header_first      = content_type() == ContentType::XML ? header_xml : header_json;
  auto header_first_size = content_type() == ContentType::XML ? header_xml_size : header_json_size;

  unsigned int body_offset = header_first_size + 20 + header_last_size;

  utils::gzip_compress_to_vector(buffer, content_length, m_buffer, body_offset);

  auto length_str   = std::to_string(m_buffer.size() - body_offset);
  auto header_size  = header_first_size + length_str.size() + header_last_size;
  auto header_start = body_offset - header_size;

  if (header_size > body_offset)
    throw torrent::internal_error("SCgiTask::gzip_response(...) header overflow : start position is negative");

  if (header_start > body_offset)
    throw torrent::internal_error("SCgiTask::gzip_response(...) header overflow : start position is beyond buffer start");

  std::memcpy(m_buffer.data() + header_start,                                         header_first, header_first_size);
  std::memcpy(m_buffer.data() + header_start + header_first_size,                     length_str.data(), length_str.size());
  std::memcpy(m_buffer.data() + header_start + header_first_size + length_str.size(), header_last, header_last_size);

  m_position = header_start;
}

} // namespace rpc
