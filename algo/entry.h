#ifndef ALGO_ENTRY_H
#define ALGO_ENTRY_H

#include <algo/bits/common.h>
#include <algo/bits/impl_entry.h>

namespace algo {

// Use a copy of the value of 't'.
template <typename Type>
inline _wrapper<_entry<typename traits<Type>::type_t> >
value(Type t) {

  return make_wrapper(_entry<typename traits<Type>::type_t>(t));
}

// Use the reference to 't'.
template <typename Type>
inline _wrapper<_entry<typename traits<Type>::ref_t> >
ref(Type& t) {

  return make_wrapper(_entry<typename traits<Type>::ref_t>(t));
}

inline _wrapper<_arg0>
_0() {
  return make_wrapper(_arg0());
}

// Use the reference to the member 't', in the class used
// as an argument when calling this functor.
template <typename Target, typename Type, typename Class>
inline _wrapper<_member<Target, Type, Class> >
member(_wrapper<Target> target, Type Class::*mem) {

  return make_wrapper(_member<Target, Type, Class>(target.m_ftor, mem));
}

template <typename Target, typename Parm0>
inline _wrapper<_on1<Target, Parm0> >
on(_wrapper<Target> target, _wrapper<Parm0> parm0) {
  
  return make_wrapper(_on1<Target, Parm0>(target.m_ftor, parm0.m_ftor));
}

}

#endif
