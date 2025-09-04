#include "config.h"

#include "http_queue.h"

#include <torrent/common.h>
#include <torrent/net/http_get.h>
#include <torrent/net/http_stack.h>

namespace core {

HttpQueue::iterator
HttpQueue::insert(const std::string& url, std::shared_ptr<std::ostream> stream) {
  auto itr = base_type::insert(end(), torrent::net::HttpGet(url, stream));

  for (auto& slot : m_signal_insert)
    slot(*itr);

  itr->add_done_slot([this, itr]() { erase(itr); });
  itr->add_failed_slot([this, itr](auto) { erase(itr); });

  // TODO: Downloading http torrents doesn't seem to work.
  // TODO: Quitting no longer works.

  torrent::net_thread::http_stack()->start_get(*itr);

  return itr;
}

void
HttpQueue::erase(iterator signal_itr) {
  for (const auto& slot : m_signal_erase)
    slot(*signal_itr);

  signal_itr->close_and_keep_callbacks();
  base_type::erase(signal_itr);
}

void
HttpQueue::clear() {
  while (!empty())
    erase(begin());
}

}
