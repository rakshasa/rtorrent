#include "config.h"

#include <torrent/torrent.h>

#include "downloads.h"

namespace engine {

void
Downloads::create(std::istream& str) {
  torrent::Download d = torrent::download_create(str);

  Base::push_back(d);
}

void
Downloads::erase(iterator itr) {
  torrent::download_remove(itr->get_hash());

  Base::erase(itr);
}

}
