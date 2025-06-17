#ifndef RTORRENT_CORE_HTTP_QUEUE_H
#define RTORRENT_CORE_HTTP_QUEUE_H

#include <functional>
#include <iosfwd>
#include <memory>
#include <list>
#include <string>
#include <torrent/net/http_get.h>

namespace core {

// TODO: Remove this.

class HttpQueue : private std::list<torrent::net::HttpGet> {
public:
  using base_type       = std::list<torrent::net::HttpGet>;
  using slot_curl_get   = std::function<void (torrent::net::HttpGet)>;
  using signal_curl_get = std::list<slot_curl_get>;

  using base_type::iterator;
  using base_type::const_iterator;
  using base_type::reverse_iterator;
  using base_type::const_reverse_iterator;

  using base_type::begin;
  using base_type::end;
  using base_type::rbegin;
  using base_type::rend;

  using base_type::empty;
  using base_type::size;

  HttpQueue() = default;
  ~HttpQueue() { clear(); }

  // Note that any slots connected to the CurlGet signals must be
  // pushed in front of the erase slot added by HttpQueue::insert.
  //
  // Consider adding a flag to indicate whetever HttpQueue should
  // delete the stream.
  iterator    insert(const std::string& url, std::shared_ptr<std::ostream> stream);
  void        erase(iterator itr);

  void        clear();

  signal_curl_get& signal_insert() { return m_signal_insert; }
  signal_curl_get& signal_erase()  { return m_signal_erase; }

private:
  signal_curl_get m_signal_insert;
  signal_curl_get m_signal_erase;
};

}

#endif
