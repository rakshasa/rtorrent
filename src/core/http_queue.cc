#include "config.h"

#include <memory>
#include <sstream>
#include <sigc++/bind.h>
#include <sigc++/hide.h>
#include <torrent/http.h>

#include "rak/functional.h"
#include "http_queue.h"
#include "curl_get.h"

namespace core {

HttpQueue::iterator
HttpQueue::insert(const std::string& url) {
  std::auto_ptr<CurlGet> h(m_slotFactory());
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

  m_signalInsert.emit(*itr);

  return itr;
}

void
HttpQueue::erase(iterator itr) {
  m_signalErase.emit(*itr);

  delete (*itr)->get_stream();
  delete *itr;

  Base::erase(itr);
}

void
HttpQueue::clear() {
  while (!empty())
    erase(begin());

  Base::clear();
}

}
