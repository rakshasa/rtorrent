#ifndef RTORRENT_CORE_DOWNLOAD_LIST_H
#define RTORRENT_CORE_DOWNLOAD_LIST_H

#include <iosfwd>

#include "download.h"

namespace core {

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

  using Base::empty;
  using Base::size;

  iterator  insert(std::istream& str);
  void      erase(iterator itr);

private:
  void      receive_hash_done(const std::string& str);
};

}

#endif
