#ifndef RTORRENT_UTILS_DIRECTORY_H
#define RTORRENT_UTILS_DIRECTORY_H

#include <string>
#include <list>

namespace utils {

class Directory : private std::list<std::string> {
public:
  typedef std::list<std::string> Base;

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

  using Base::erase;

  Directory() {}
  Directory(const std::string& path) : m_path(path) {}

  void                update();

  const std::string&  get_path() { return m_path; }

  // Make a list with full path names.
  Base                make_list();

private:
  std::string         m_path;
};

}

#endif
