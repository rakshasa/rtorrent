#include "config.h"

#include <algorithm>
#include <stdexcept>
#include <sigc++/bind.h>
#include <torrent/exceptions.h>
#include <torrent/torrent.h>

#include "download_list.h"

namespace core {

DownloadList::iterator
DownloadList::insert(std::istream* str) {
  torrent::Download d = torrent::download_create(str);

  iterator itr = Base::insert(end(), Download());
  itr->set_download(d);

  return itr;
}

void
DownloadList::erase(iterator itr) {
  itr->release_download();

  torrent::download_remove(itr->get_hash());
  Base::erase(itr);
}

}
