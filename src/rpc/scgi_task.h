#ifndef RTORRENT_RPC_SCGI_TASK_H
#define RTORRENT_RPC_SCGI_TASK_H

#include <memory>
#include <mutex>
#include <torrent/event.h>

namespace rpc {

class SCgi;

class SCgiTask : public torrent::Event {
public:
  static constexpr int default_buffer_size = 8191;
  static constexpr int max_header_size     = 2000;
  static constexpr int max_content_size    = (2 << 23);

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
  static constexpr char header_xml[]  = "Status: 200 OK\r\nContent-Type: text/xml\r\nContent-Length: ";
  static constexpr char header_json[] = "Status: 200 OK\r\nContent-Type: application/json\r\nContent-Length: ";
  static constexpr char header_last[] = "\r\n\r\n";

  static constexpr size_t header_xml_size  = sizeof(header_xml) - 1;
  static constexpr size_t header_json_size = sizeof(header_json) - 1;
  static constexpr size_t header_last_size = sizeof(header_last) - 1;

  bool                parse_headers(const char* current, unsigned int header_length);
  bool                detect_content_type(const std::string& content_type);

  void                receive_call(const char* buffer, uint32_t length);
  void                receive_write(const char* buffer, uint32_t length);

  void                plaintext_response(const char* buffer, uint32_t content_length);
  void                gzip_response(const char* buffer, uint32_t content_length);

  SCgi*               m_parent{};

  std::mutex          m_result_mutex;

  std::vector<char>   m_buffer;
  unsigned int        m_position{};
  unsigned int        m_body{};

  unsigned int        m_content_length{};
  ContentType         m_content_type{XML};

  bool                m_accepts_compression{};
};

}

#endif
