#ifndef RTORRENT_UTILS_ALGORITHM_H
#define RTORRENT_UTILS_ALGORITHM_H

#include <algorithm>

namespace utils {

template <typename _InputIter, typename _Function>
_Function for_each_pre(_InputIter __first, _InputIter __last, _Function __f) {
  _InputIter __tmp;

  while (__first != __last) {
    __tmp = first++;
    
    __f(*__tmp);
  }

  return __f;
}

}

#endif
