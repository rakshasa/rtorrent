// rTorrent - BitTorrent client
// Copyright (C) 2005, Jari Sundell
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
// Contact:  Jari Sundell <jaris@ifi.uio.no>
//
//           Skomakerveien 33
//           3185 Skoppum, NORWAY

#ifndef RTORRENT_UTILS_FUNCTIONAL_H
#define RTORRENT_UTILS_FUNCTIONAL_H

#include <functional>

namespace rak {

template <typename Type, typename Ftor>
struct _accumulate {
  _accumulate(Type& t, Ftor f) : m_t(t), m_f(f) {}

  template <typename Arg>
  void operator () (Arg& a) { m_t += m_f(a); }

  Type& m_t;
  Ftor m_f;
};

template <typename Type, typename Ftor>
inline _accumulate<Type, Ftor>
accumulate(Type& t, Ftor f) {
  return _accumulate<Type, Ftor>(t, f);
}

template <typename Type, typename Ftor>
struct _equal {
  _equal(Type t, Ftor f) : m_t(t), m_f(f) {}

  template <typename Arg>
  bool operator () (Arg& a) {
    return m_t == m_f(a);
  }

  Type m_t;
  Ftor m_f;
};

template <typename Type, typename Ftor>
inline _equal<Type, Ftor>
equal(Type t, Ftor f) {
  return _equal<Type, Ftor>(t, f);
}

template <typename Src, typename Dest>
struct _on : public std::unary_function<typename Src::argument_type, typename Dest::result_type>  {
  _on(Src s, Dest d) : m_dest(d), m_src(s) {}

  typename Dest::result_type operator () (typename Src::argument_type arg) {
    return m_dest(m_src(arg));
  }

  Dest m_dest;
  Src m_src;
};
    
template <typename Src, typename Dest>
inline _on<Src, Dest>
on(Src s, Dest d) {
  return _on<Src, Dest>(s, d);
}  

// Creates a functor for accessing a member.
template <typename Class, typename Member>
struct _mem_ptr_ref : public std::unary_function<Class&, Member&> {
  _mem_ptr_ref(Member Class::*m) : m_member(m) {}

  Member& operator () (Class& c) {
    return c.*m_member;
  }

  Member Class::*m_member;
};

template <typename Class, typename Member>
inline _mem_ptr_ref<Class, Member>
mem_ptr_ref(Member Class::*m) {
  return _mem_ptr_ref<Class, Member>(m);
}

template <typename Cond, typename Then>
struct _if_then {
  _if_then(Cond c, Then t) : m_cond(c), m_then(t) {}

  template <typename Arg>
  void operator () (Arg& a) {
    if (m_cond(a))
      m_then(a);
  }

  Cond m_cond;
  Then m_then;
};

template <typename Cond, typename Then>
inline _if_then<Cond, Then>
if_then(Cond c, Then t) {
  return _if_then<Cond, Then>(c, t);
}

template <typename T>
struct call_delete : public std::unary_function<T*, void> {
  void operator () (T* t) {
    delete t;
  }
};

}

#endif
