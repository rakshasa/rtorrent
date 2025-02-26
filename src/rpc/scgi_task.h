#ifndef RTORRENT_RPC_SCGI_TASK_H
#define RTORRENT_RPC_SCGI_TASK_H

#include <string>
#include <torrent/event.h>
#include <vector>

namespace utils {
class SocketFd;
}

namespace rpc {

class SCgi;

class SCgiTask : public torrent::Event {
public:
  static constexpr unsigned int default_buffer_size = 2047;
  static constexpr int          max_header_size     = 2000;
  static constexpr int          max_content_size    = (2 << 23);

  static int                    gzip_min_size() { return m_min_compress_response_size; }
  static void                   set_gzip_min_size(int size) { m_min_compress_response_size = size; }
  static bool                   gzip_enabled() { return m_allow_compressed_response; }
  static void                   set_gzip_enabled(bool enabled) { m_allow_compressed_response = enabled; }

  enum ContentType { XML,
                     JSON };

  SCgiTask() { m_fileDesc = -1; }

  bool             is_open() const { return m_fileDesc != -1; }
  bool             is_available() const { return m_fileDesc == -1; }

  void             open(SCgi* parent, int fd);
  void             close();

  ContentType      content_type() const { return m_content_type; }

  virtual void     event_read();
  virtual void     event_write();
  virtual void     event_error();

  bool             receive_write(const char* buffer, uint32_t length);

  utils::SocketFd& get_fd() { return *reinterpret_cast<utils::SocketFd*>(&m_fileDesc); }

private:
  int              read_header();

  void             gzip_compress_response(const char* buffer, uint32_t length);
  void             plaintext_response(const char* buffer, uint32_t length);

  static bool      m_allow_compressed_response;
  static int       m_min_compress_response_size;

  SCgi*            m_parent;

  std::vector<char> m_buffer;

  // TODO: Change to index.
  char*             m_position;
  char*             m_body;

  ContentType      m_content_type{XML};
  bool             m_client_accepts_compressed_response{false};
};

} // namespace rpc

#endif
