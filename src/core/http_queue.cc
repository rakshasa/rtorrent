#include "config.h"

#include <memory>
#include <sstream>
#include <sigc++/bind.h>
#include <sigc++/hide.h>
#include <torrent/http.h>

#include "functional.h"
#include "http_queue.h"
#include "curl_get.h"

namespace core {

HttpQueue::iterator
HttpQueue::insert(const std::string& url) {
  std::auto_ptr<torrent::Http> h(m_slotFactory());
  std::auto_ptr<std::stringstream> s(new std::stringstream);
  
  h->set_url(url);
  h->set_stream(s.get());
  h->set_user_agent("rtorrent/" VERSION);

  iterator itr = Base::insert(end(), h.get());

  h->signal_done().connect(sigc::bind(sigc::mem_fun(this, &HttpQueue::erase), itr));
  h->signal_failed().connect(sigc::bind<0>(sigc::hide(sigc::mem_fun(this, &HttpQueue::erase)), itr));

  (*itr)->start();

  h.release();
  s.release();

  return itr;
}

void
HttpQueue::erase(iterator itr) {
  delete (*itr)->get_stream();
  delete *itr;

  Base::erase(itr);
}

void
HttpQueue::clear() {
  std::for_each(begin(), end(), func::on(func::call_delete(), std::mem_fun(&CurlGet::get_stream)));
  std::for_each(begin(), end(), func::call_delete());

  Base::clear();
}

}
