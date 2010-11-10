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

// Some allocators for cacheline aligned chunks of memory, etc.

#ifndef RAK_ALLOCATORS_H
#define RAK_ALLOCATORS_H

#include <cstddef>
#include <limits>
#include <stdlib.h>
#include <sys/types.h>

namespace rak {

template <class T = void*>
class cacheline_allocator {
public:
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;
  typedef T* pointer;
  typedef const T* const_pointer;
  typedef const void* const_void_pointer;
  typedef T& reference;
  typedef const T& const_reference;
  typedef T value_type;

  cacheline_allocator() throw() { }
  cacheline_allocator(const cacheline_allocator&) throw() { }
  template <class U>
  cacheline_allocator(const cacheline_allocator<U>&) throw() { }
  ~cacheline_allocator() throw() { }

  template <class U>
  struct rebind { typedef cacheline_allocator<U> other; };

  // return address of values
  pointer address (reference value) const { return &value; }
  const_pointer address (const_reference value) const { return &value; }

  size_type max_size () const throw() { return std::numeric_limits<size_t>::max() / sizeof(T); }

  pointer allocate(size_type num, const_void_pointer hint = 0) { return alloc_size(num*sizeof(T)); }

  static pointer alloc_size(size_type size) {
    pointer ptr = NULL;
    int __UNUSED result = posix_memalign((void**)&ptr, LT_SMP_CACHE_BYTES, size);

    return ptr;
  }

  void construct (pointer p, const T& value) { new((void*)p)T(value); }
  void destroy (pointer p) { p->~T(); }
  void deallocate (pointer p, size_type num) { free((void*)p); }
};


template <class T1, class T2>
bool operator== (const cacheline_allocator<T1>&, const cacheline_allocator<T2>&) throw() {
  return true;
}

template <class T1, class T2>
bool operator!= (const cacheline_allocator<T1>&, const cacheline_allocator<T2>&) throw() {
  return false;
}

}

//
// Operator new with custom allocators:
//

template <typename T>
void* operator new(size_t s, rak::cacheline_allocator<T> a) { return a.alloc_size(s); }

#endif // namespace rak
