#include "config.h"

#include <memory>
#include <sstream>
#include <sigc++/bind.h>
#include <torrent/http.h>

#include "functional.h"
#include "http_queue.h"
#include "curl_get.h"

namespace core {

void
HttpQueue::insert(const std::string& url) {
  std::auto_ptr<torrent::Http> h(m_slotFactory());
  std::auto_ptr<std::stringstream> s(new std::stringstream);
  
  h->set_out(s.get());
  h->set_url(url);
  h->set_user_agent("rtorrent/" VERSION);

  iterator itr = Base::insert(end(), h.get());

  h->slot_done(sigc::bind(sigc::mem_fun(this, &HttpQueue::receive_done), itr));
  h->slot_failed(sigc::bind<0>(sigc::mem_fun(this, &HttpQueue::receive_failed), itr));

  (*itr)->start();

  h.release();
  s.release();
}

void
HttpQueue::erase(iterator itr) {
  delete (*itr)->get_out();
  delete *itr;

  Base::erase(itr);
}

void
HttpQueue::clear() {
  std::for_each(begin(), end(), func::on(func::call_delete(), std::mem_fun(&CurlGet::get_out)));
  std::for_each(begin(), end(), func::call_delete());

  Base::clear();
}

void
HttpQueue::receive_done(iterator itr) {
}

void
HttpQueue::receive_failed(iterator itr, std::string msg) {
}

}
