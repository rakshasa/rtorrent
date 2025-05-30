#ifndef RTORRENT_CORE_CURL_GET_H
#define RTORRENT_CORE_CURL_GET_H

#include <iosfwd>
#include <string>
#include <curl/curl.h>
#include <torrent/http.h>
#include <torrent/utils/scheduler.h>

namespace core {

class CurlStack;

class CurlGet : public torrent::Http {
public:
  CurlGet(CurlStack* s);
  virtual ~CurlGet();

  void               start();
  void               close();

  bool               is_using_ipv6()    { return m_ipv6; }
  void               retry_ipv6();

  bool               is_busy() const    { return m_handle; }
  bool               is_active() const  { return m_active; }

  void               set_active(bool a) { m_active = a; }

  double             size_done();
  double             size_total();

  CURL*              handle()           { return m_handle; }

private:
  friend class CurlStack;

  CurlGet(const CurlGet&) = delete;
  void operator = (const CurlGet&) = delete;

  void               receive_timeout();

  bool               m_active{};
  bool               m_ipv6;

  torrent::utils::SchedulerEntry m_task_timeout;

  CURL*              m_handle{};
  CurlStack*         m_stack;
};

}

#endif
