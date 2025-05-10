#include "config.h"

#include <memory>
#include <sstream>
#include <torrent/http.h>

#include "http_queue.h"
#include "curl_get.h"

namespace core {

HttpQueue::iterator
HttpQueue::insert(const std::string& url, std::iostream* s) {
  std::unique_ptr<CurlGet> h(m_slot_factory());

  h->set_url(url);
  h->set_stream(s);
  h->set_timeout(5 * 60);

  iterator signal_itr = base_type::insert(end(), h.get());

  h->signal_done().push_back(std::bind(&HttpQueue::erase, this, signal_itr));
  h->signal_failed().push_back(std::bind(&HttpQueue::erase, this, signal_itr));

  (*signal_itr)->start();

  h.release();

  for (signal_curl_get::iterator itr = m_signal_insert.begin(), last = m_signal_insert.end(); itr != last; itr++)
    (*itr)(*signal_itr);

  return signal_itr;
}

void
HttpQueue::erase(iterator signal_itr) {
  for (signal_curl_get::iterator itr = m_signal_erase.begin(), last = m_signal_erase.end(); itr != last; itr++)
    (*itr)(*signal_itr);

  delete *signal_itr;
  base_type::erase(signal_itr);
}

void
HttpQueue::clear() {
  while (!empty())
    erase(begin());

  base_type::clear();
}

}
