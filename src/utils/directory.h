#ifndef RTORRENT_UTILS_DIRECTORY_H
#define RTORRENT_UTILS_DIRECTORY_H

#include <string>
#include <vector>

namespace utils {

class Directory : private std::vector<std::string> {
public:
  typedef std::vector<std::string> Base;

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

  Directory() {}
  Directory(const std::string& path) : m_path(path) {}

  void                update();

  const std::string&  get_path() { return m_path; }

private:
  std::string         m_path;
};

}

#endif
