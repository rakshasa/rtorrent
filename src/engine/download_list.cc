#include "config.h"

#include <algorithm>
#include <sigc++/bind.h>
#include <torrent/exceptions.h>
#include <torrent/torrent.h>

#include "download_list.h"

namespace engine {

DownloadList::iterator
DownloadList::create(std::istream& str) {
  iterator itr = Base::insert(end(), torrent::download_create(str));

  itr->get_download().signal_hash_done(sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_hash_done),
						  itr->get_hash()));

  return itr;
}

void
DownloadList::erase(iterator itr) {
  torrent::download_remove(itr->get_hash());

  Base::erase(itr);
}

void
DownloadList::receive_hash_done(const std::string& str) {
  iterator itr = std::find(begin(), end(), str);

  if (itr == end())
    throw torrent::client_error("DownloadList received hash check done, but couldn't find the download");

  itr->get_download().start();
}

}
