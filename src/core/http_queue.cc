#include "config.h"

#include "http_queue.h"

#include <torrent/net/http_get.h>
#include <torrent/net/http_stack.h>

namespace core {

HttpQueue::iterator
HttpQueue::insert(const std::string& url, std::iostream* s) {
  auto itr = base_type::insert(end(), torrent::net_thread::http_stack()->create(url, s));

  itr->add_done_slot([this, itr]() { erase(itr); });
  itr->add_failed_slot([this, itr](auto) { erase(itr); });

  itr->start();

  for (auto& slot : m_signal_insert)
    slot(*itr);

  return itr;
}

void
HttpQueue::erase(iterator signal_itr) {
  for (const auto& slot : m_signal_erase)
    slot(*signal_itr);

  base_type::erase(signal_itr);
}

void
HttpQueue::clear() {
  while (!empty())
    erase(begin());
}

}
