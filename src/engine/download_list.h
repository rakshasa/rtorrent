#ifndef RTORRENT_ENGINE_DOWNLOAD_LIST_H
#define RTORRENT_ENGINE_DOWNLOAD_LIST_H

#include <iosfwd>

#include "download.h"

namespace engine {

class DownloadList : private std::list<Download> {
public:
  typedef std::list<Download> Base;

  using Base::iterator;
  using Base::const_iterator;
  using Base::reverse_iterator;
  using Base::const_reverse_iterator;

  using Base::begin;
  using Base::end;
  using Base::rbegin;
  using Base::rend;

  iterator  create(std::istream& str);
  void      erase(iterator itr);

private:
  void      receive_hash_done(const std::string& str);
};

}

#endif
