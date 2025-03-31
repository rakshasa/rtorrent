#ifndef RTORRENT_RPC_SCGI_TASK_H
#define RTORRENT_RPC_SCGI_TASK_H

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

  bool                receive_write(const char* buffer, uint32_t length);

  utils::SocketFd&    get_fd()            { return *reinterpret_cast<utils::SocketFd*>(&m_fileDesc); }

private:
  inline void         realloc_buffer(uint32_t size, const char* buffer, uint32_t bufferSize);

  SCgi*               m_parent;

  char*               m_buffer;
  char*               m_position;
  char*               m_body;

  unsigned int        m_bufferSize;

  ContentType         m_content_type{ XML };
};

}

#endif
