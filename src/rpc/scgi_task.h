#ifndef RTORRENT_RPC_SCGI_TASK_H
#define RTORRENT_RPC_SCGI_TASK_H

#include <memory>
#include <mutex>
#include <torrent/event.h>

namespace utils {
  class SocketFd;
}

namespace rpc {

class SCgi;

class SCgiTask : public torrent::Event {
public:
  static const unsigned int default_buffer_size = 2047;
  static const          int max_header_size     = 2000;
  static const          int max_content_size    = (2 << 23);

  enum ContentType { XML, JSON };

  SCgiTask() { m_fileDesc = -1; }

  bool                is_open() const      { return m_fileDesc != -1; }
  bool                is_available() const { return m_fileDesc == -1; }

  void                open(SCgi* parent, int fd);
  void                close();

  ContentType         content_type() const { return m_content_type; }

  virtual void        event_read();
  virtual void        event_write();
  virtual void        event_error();

  utils::SocketFd&    get_fd()             { return *reinterpret_cast<utils::SocketFd*>(&m_fileDesc); }

private:
  bool                detect_content_type(const std::string& content_type);
  void                realloc_buffer(uint32_t size, const char* buffer, uint32_t bufferSize);

  void                receive_call(const char* buffer, uint32_t length);
  void                receive_write(const char* buffer, uint32_t length);

  SCgi*               m_parent;

  std::mutex          m_result_mutex;

  char*               m_buffer{nullptr};
  char*               m_position{nullptr};
  char*               m_body{nullptr};

  unsigned int        m_buffer_size{0};

  ContentType         m_content_type{ XML };
};

}

#endif
