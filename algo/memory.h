// -*- c++ -*-
/* Do not edit! -- generated file */

#ifndef ALGO_MEMORY_H
#define ALGO_MEMORY_H

#include <algo/bits/common.h>
#include <algo/bits/impl_memory.h>

namespace algo {

template <typename Ftor>
inline _wrapper<_delete<Ftor> >
call_delete(_wrapper<Ftor> w) {
  return make_wrapper(_delete<Ftor>(w.m_ftor));
}

template <typename Type>
inline _wrapper<_new0<Type> >
call_new() {
  return make_wrapper(_new0<Type >());
}

template <typename Type, typename P1>
inline _wrapper<_new1<Type, P1> >
call_new(_wrapper<P1> p1) {
  return make_wrapper(_new1<Type, P1 >(p1.m_ftor));
}

template <typename Type, typename P1,typename P2>
inline _wrapper<_new2<Type, P1,P2> >
call_new(_wrapper<P1> p1,_wrapper<P2> p2) {
  return make_wrapper(_new2<Type, P1,P2 >(p1.m_ftor,p2.m_ftor));
}

template <typename Type, typename P1,typename P2,typename P3>
inline _wrapper<_new3<Type, P1,P2,P3> >
call_new(_wrapper<P1> p1,_wrapper<P2> p2,_wrapper<P3> p3) {
  return make_wrapper(_new3<Type, P1,P2,P3 >(p1.m_ftor,p2.m_ftor,p3.m_ftor));
}

template <typename Type, typename P1,typename P2,typename P3,typename P4>
inline _wrapper<_new4<Type, P1,P2,P3,P4> >
call_new(_wrapper<P1> p1,_wrapper<P2> p2,_wrapper<P3> p3,_wrapper<P4> p4) {
  return make_wrapper(_new4<Type, P1,P2,P3,P4 >(p1.m_ftor,p2.m_ftor,p3.m_ftor,p4.m_ftor));
}


}

#endif
