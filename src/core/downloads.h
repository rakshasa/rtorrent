#ifndef RTORRENT_ENGINE_DOWNLOADS_H
#define RTORRENT_ENGINE_DOWNLOADS_H

#include <iosfwd>
#include <torrent/download.h>

namespace engine {

class Downloads : private std::list<torrent::Download> {
public:
  typedef std::list<torrent::Download> Base;

  using Base::iterator;
  using Base::const_iterator;
  using Base::reverse_iterator;
  using Base::const_reverse_iterator;

  using Base::begin;
  using Base::end;
  using Base::rbegin;
  using Base::rend;

  void create(std::istream& str);
  void erase(iterator itr);
};

}

#endif
