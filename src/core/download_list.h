#ifndef RTORRENT_CORE_DOWNLOAD_LIST_H
#define RTORRENT_CORE_DOWNLOAD_LIST_H

#include <iosfwd>
#include <list>

namespace core {

class Download;

class DownloadList : private std::list<Download*> {
public:
  typedef std::list<Download*> Base;

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

  ~DownloadList() { clear(); }

  iterator  insert(std::istream* str);
  void      erase(iterator itr);

private:
  void      clear();
};

}

#endif
