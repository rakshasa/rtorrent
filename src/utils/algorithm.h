#ifndef RTORRENT_UTILS_ALGORITHM_H
#define RTORRENT_UTILS_ALGORITHM_H

#include <algorithm>

namespace utils {

template <typename _InputIter, typename _Function>
_Function
for_each_pre(_InputIter __first, _InputIter __last, _Function __f) {
  _InputIter __tmp;

  while (__first != __last) {
    __tmp = __first++;
    
    __f(*__tmp);
  }

  return __f;
}

// Return a range with a distance of no more than __distance and
// between __first and __last, centered on __middle1.
template <typename _InputIter, typename _Distance>
std::pair<_InputIter, _InputIter>
advance_bidirectional(_InputIter __first, _InputIter __middle1, _InputIter __last, _Distance __distance) {
  _InputIter __middle2 = __middle1;

  do {
    if (!__distance)
      break;

    if (__middle1 != __first) {
      --__middle1;
      --__distance;

    } else if (__middle2 == __last) {
      break;
    }

    if (!__distance)
      break;

    if (__middle2 != __last) {
      ++__middle2;
      --__distance;

    } else if (__middle1 == __first) {
      break;
    }
  } while (true);

  return std::make_pair(__middle1, __middle2);
}

}

#endif
