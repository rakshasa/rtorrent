#include "config.h"

#include <algorithm>
#include <torrent/torrent.h>

#include "rak/functional.h"

#include "download.h"
#include "download_list.h"

namespace core {

DownloadList::iterator
DownloadList::insert(std::istream* str) {
  torrent::Download d = torrent::download_create(str);

  iterator itr = Base::insert(end(), new Download);
  (*itr)->set_download(d);

  return itr;
}

DownloadList::iterator
DownloadList::erase(iterator itr) {
  (*itr)->release_download();

  torrent::download_remove((*itr)->get_hash());
  delete *itr;

  return Base::erase(itr);
}

void
DownloadList::clear() {
  std::for_each(begin(), end(), rak::call_delete<Download>());

  Base::clear();
}

}
