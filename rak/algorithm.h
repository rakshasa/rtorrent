// rak - Rakshasa's toolbox
// Copyright (C) 2005-2007, Jari Sundell
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// In addition, as a special exception, the copyright holders give
// permission to link the code of portions of this program with the
// OpenSSL library under certain conditions as described in each
// individual source file, and distribute linked combinations
// including the two.
//
// You must obey the GNU General Public License in all respects for
// all of the code used other than OpenSSL.  If you modify file(s)
// with this exception, you may extend this exception to your version
// of the file(s), but you are not obligated to do so.  If you do not
// wish to do so, delete this exception statement from your version.
// If you delete this exception statement from all source files in the
// program, then also delete it here.
//
// Contact:  Jari Sundell <jaris@ifi.uio.no>
//
//           Skomakerveien 33
//           3185 Skoppum, NORWAY

#ifndef RAK_ALGORITHM_H
#define RAK_ALGORITHM_H

#include <algorithm>
#include <functional>
#include <limits>

namespace rak {

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

    if (__middle2 != __last) {
      ++__middle2;
      --__distance;

    } else if (__middle1 == __first) {
      break;
    }

    if (!__distance)
      break;

    if (__middle1 != __first) {
      --__middle1;
      --__distance;

    } else if (__middle2 == __last) {
      break;
    }

  } while (true);

  return std::make_pair(__middle1, __middle2);
}

template <typename _InputIter, typename _Distance>
_InputIter
advance_forward(_InputIter __first, _InputIter __last, _Distance __distance) {
  while (__first != __last && __distance != 0) {
    __first++;
    __distance--;
  }

  return __first;
}

template <typename _InputIter, typename _Distance>
_InputIter
advance_backward(_InputIter __first, _InputIter __last, _Distance __distance) {
  while (__first != __last && __distance != 0) {
    __first--;
    __distance--;
  }

  return __first;
}

template <typename _Value>
struct compare_base : public std::function<bool(_Value, _Value)> {
  bool operator () (const _Value& complete, const _Value& base) const {
    return !complete.compare(0, base.size(), base);
  }
};

// Count the number of elements from the start of the containers to
// the first inequal element.
template <typename _InputIter1, typename _InputIter2>
typename std::iterator_traits<_InputIter1>::difference_type
count_base(_InputIter1 __first1, _InputIter1 __last1,
	   _InputIter2 __first2, _InputIter2 __last2) {

  typename std::iterator_traits<_InputIter1>::difference_type __n = 0;

  for ( ;__first1 != __last1 && __first2 != __last2; ++__first1, ++__first2, ++__n)
    if (*__first1 != *__first2)
      return __n;

  return __n;
}

template <typename _Return, typename _InputIter, typename _Ftor>
_Return
make_base(_InputIter __first, _InputIter __last, _Ftor __ftor) {
  if (__first == __last)
    return "";

  _Return __base = __ftor(*__first++);

  for ( ;__first != __last; ++__first) {
    typename std::iterator_traits<_InputIter>::difference_type __pos = count_base(__base.begin(), __base.end(),
										  __ftor(*__first).begin(), __ftor(*__first).end());

    if (__pos < (typename std::iterator_traits<_InputIter>::difference_type)__base.size())
      __base.resize(__pos);
  }

  return __base;
}

template<typename T>
inline int popcount_wrapper(T t) {
#if USE_BUILTIN_POPCOUNT
  return __builtin_popcountll(t);
#else
#error __builtin_popcount not found.
  unsigned int count = 0;
  
  while (t) {
    count += t & 0x1;
    t >> 1;
  }

  return count;
#endif
}

// Get the median of an unordered set of numbers of arbitrary
// type by modifing the underlying dataset
template <typename T = double, typename _InputIter>
T median(_InputIter __first, _InputIter __last) {
  T __med;

  unsigned int __size = __last - __first;
  unsigned int __middle = __size / 2;
  _InputIter __target1 = __first + __middle;
  std::nth_element(__first, __target1, __last);
  __med = *__target1;

  if (__size % 2 == 0) {
    _InputIter __target2 = std::max_element(__first, __target1);
    __med = (__med + *__target2) / 2.0;
  }

  return __med;
}

}

#endif
