#ifndef RTORRENT_RPC_SCGI_TASK_H
#define RTORRENT_RPC_SCGI_TASK_H

#include <memory>
#include <mutex>
#include <torrent/event.h>

namespace rpc {

class SCgi;

class SCgiTask : public torrent::Event {
public:
  static const unsigned int default_buffer_size = 2047;
  static const          int max_header_size     = 2000;
  static const          int max_content_size    = (2 << 23);

  enum ContentType { XML, JSON };

  SCgiTask() { m_fileDesc = -1; }

  const char*         type_name() const override { return "scgi-task"; }

  bool                is_open() const      { return m_fileDesc != -1; }
  bool                is_available() const { return m_fileDesc == -1; }

  void                open(SCgi* parent, int fd);
  void                cancel_open();

  void                close();

  ContentType         content_type() const { return m_content_type; }

  void                event_read() override;
  void                event_write() override;
  void                event_error() override;

private:
  bool                detect_content_type(const std::string& content_type);
  void                realloc_buffer(uint32_t size, const char* buffer, uint32_t bufferSize);

  void                receive_call(const char* buffer, uint32_t length);
  void                receive_write(const char* buffer, uint32_t length);

  SCgi*               m_parent{};

  std::mutex          m_result_mutex;

  std::unique_ptr<char[]> m_buffer;
  char*                   m_position{};
  char*                   m_body{};

  unsigned int        m_buffer_size{0};

  ContentType         m_content_type{ XML };
};

}

#endif
