#ifndef RTORRENT_CORE_CURL_STACK_H
#define RTORRENT_CORE_CURL_STACK_H

#include <deque>
#include <string>
#include <torrent/utils/scheduler.h>

namespace core {

class CurlGet;
class CurlSocket;

// By using a deque instead of vector we allow for cheaper removal of
// the oldest elements, those that will be first in the in the
// deque.
//
// This should fit well with the use-case of a http stack, thus
// we get most of the cache locality benefits of a vector with fast
// removal of elements.

class CurlStack : std::deque<CurlGet*> {
public:
  friend class CurlGet;

  typedef std::deque<CurlGet*> base_type;

  using base_type::value_type;
  using base_type::iterator;
  using base_type::const_iterator;
  using base_type::reverse_iterator;
  using base_type::const_reverse_iterator;

  using base_type::begin;
  using base_type::end;
  using base_type::rbegin;
  using base_type::rend;

  using base_type::back;
  using base_type::front;

  using base_type::size;
  using base_type::empty;

  CurlStack();
  ~CurlStack();

  void                shutdown();
  bool                is_running() const                     { return m_running; }

  CurlGet*            new_object();
  CurlSocket*         new_socket(int fd);

  unsigned int        active() const                         { return m_active; }
  unsigned int        max_active() const                     { return m_max_active; }
  void                set_max_active(unsigned int a)         { m_max_active = a; }

  const std::string&  user_agent() const                     { return m_user_agent; }
  const std::string&  http_proxy() const                     { return m_http_proxy; }
  const std::string&  bind_address() const                   { return m_bind_address; }
  const std::string&  http_capath() const                    { return m_http_ca_path; }
  const std::string&  http_cacert() const                    { return m_http_ca_cert; }

  void                set_user_agent(const std::string& s)   { m_user_agent = s; }
  void                set_http_proxy(const std::string& s)   { m_http_proxy = s; }
  void                set_bind_address(const std::string& s) { m_bind_address = s; }
  void                set_http_capath(const std::string& s)  { m_http_ca_path = s; }
  void                set_http_cacert(const std::string& s)  { m_http_ca_cert = s; }

  bool                ssl_verify_host() const                { return m_ssl_verify_host; }
  bool                ssl_verify_peer() const                { return m_ssl_verify_peer; }
  void                set_ssl_verify_host(bool s)            { m_ssl_verify_host = s; }
  void                set_ssl_verify_peer(bool s)            { m_ssl_verify_peer = s; }

  long                dns_timeout() const                    { return m_dns_timeout; }
  void                set_dns_timeout(long timeout)          { m_dns_timeout = timeout; }

  static void         global_init();
  static void         global_cleanup();

  void                receive_action(CurlSocket* socket, int type);

  static int          set_timeout(void* handle, std::chrono::microseconds timeout, void* userp);

  void                transfer_done(void* handle, const char* msg);

protected:
  void                add_get(CurlGet* get);
  void                remove_get(CurlGet* get);

private:
  CurlStack(const CurlStack&) = delete;
  void operator = (const CurlStack&) = delete;

  void                receive_timeout();

  bool                process_done_handle();

  void*               m_handle;

  bool                m_running{true};

  unsigned int        m_active{0};
  unsigned int        m_max_active{32};

  torrent::utils::SchedulerEntry m_task_timeout;

  std::string         m_user_agent;
  std::string         m_http_proxy;
  std::string         m_bind_address;
  std::string         m_http_ca_path;
  std::string         m_http_ca_cert;

  bool                m_ssl_verify_host{true};
  bool                m_ssl_verify_peer{true};
  long                m_dns_timeout{60};
};

}

#endif
