#include "config.h"

#include "http_queue.h"

#include <torrent/common.h>
#include <torrent/net/http_get.h>
#include <torrent/net/http_stack.h>

namespace core {

HttpQueue::iterator
HttpQueue::insert(const std::string& url, std::shared_ptr<std::ostream> stream,
                  std::function<void()> done_fn, std::function<void(const std::string&)> failed_fn) {
  auto itr = base_type::insert(end(), torrent::net::HttpGet(url, stream));

  itr->set_max_file_size(15 << 20);
  itr->set_redirect_only_http_https();

  for (auto& slot : m_signal_insert)
    slot(*itr);

  itr->add_done_slot(torrent::this_thread::thread(),   std::move(done_fn));
  itr->add_done_slot(torrent::this_thread::thread(),   [this, itr]() { erase(itr); });

  itr->add_failed_slot(torrent::this_thread::thread(), std::move(failed_fn));
  itr->add_failed_slot(torrent::this_thread::thread(), [this, itr](auto) { erase(itr); });

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
