#include "config.h"

#include <algorithm>
#include <stdexcept>
#include <sigc++/bind.h>

#include "download.h"
#include "functional.h"
#include "hash_queue.h"

namespace core {

void
HashQueue::insert(Download* d, Slot s) {
  if (d->get_download().is_hash_checking())
    return;

  if (std::find_if(begin(), end(), func::equal(d, std::mem_fun(&HashQueueNode::get_download))) != end())
    throw std::logic_error("core::HashQueue::insert(...) received a Download that is already queued");

  if (d->get_download().is_hash_checked()) {
    s();
    return;
  }

  iterator itr = Base::insert(end(), new HashQueueNode(d, s));

  (*itr)->set_connection(d->get_download().signal_hash_done(sigc::bind(sigc::mem_fun(*this, &HashQueue::receive_hash_done), itr)));

  fill_queue();
}

void
HashQueue::remove(Download* d) {
  iterator itr = std::find_if(begin(), end(), func::equal(d, std::mem_fun(&HashQueueNode::get_download)));

  if (itr == end())
    return;

  if ((*itr)->get_download()->get_download().is_hash_checking())
    // What do we do if we're already checking?
    ;

  delete *itr;
  Base::erase(itr);

  fill_queue();
}

void
HashQueue::receive_hash_done(Base::iterator itr) {
  Slot s = (*itr)->get_slot();

  delete *itr;
  Base::erase(itr);

  s();

  fill_queue();
}

void
HashQueue::fill_queue() {
  if (empty() || front()->get_download()->get_download().is_hash_checking())
    return;

  if (front()->get_download()->get_download().is_hash_checked())
    throw std::logic_error("core::HashQueue::fill_queue() encountered a checked hash");
  
  front()->get_download()->get_download().hash_check();
}

}
