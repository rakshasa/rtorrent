#ifndef ALGO_BITS_IMPL_ENTRY_H
#define ALGO_BITS_IMPL_ENTRY_H

namespace algo {

template <typename Type>
struct _entry {
  
  template <typename Arg0>							
  struct _ {
    typedef Type return_t;
  };

  _entry(Type t) : m_t(t) {}

  template <typename Arg0>
  typename _<Arg0&>::return_t operator () (Arg0& arg0) {
    return m_t;
  }

  template <typename Arg0>
  typename _<const Arg0&>::return_t operator () (const Arg0& arg0) {
    return m_t;
  }

  Type m_t;
};

template <typename Target, typename Type, typename Class>
struct _member {

  template <typename Arg0>
  struct _ {
    typedef typename traits<Type>::ref_t return_t;
  };

  _member(Target target, Type Class::*member) :
    m_target(target), m_member(member) {}

  template <typename Arg0>
  typename _<Arg0&>::return_t operator () (Arg0& arg0) {
    return (m_target(arg0)).*m_member;
  }

  template <typename Arg0>
  typename _<const Arg0&>::return_t operator () (const Arg0& arg0) {
    return (m_target(arg0)).*m_member;
  }

  Target m_target;
  Type Class::*m_member;
};

struct _arg0 {

  template <typename Arg0>
  struct _ {
    typedef typename traits<Arg0>::ref_t return_t;
  };

  template <typename Arg0>
  typename _<Arg0&>::return_t operator () (Arg0& arg0) {
    return arg0;
  }

  template <typename Arg0>
  typename _<const Arg0&>::return_t operator () (const Arg0& arg0) {
    return arg0;
  }
};

// Consider implementing _on0 and _on2 as m4 macros, but there will
// never be more than 0-2, so dunno.
template <typename Target, typename Parm0>
struct _on1 {
  
  template <typename Arg0>
  struct _ {
    typedef typeof(_makeT<Target>()(_makeT<Arg0>())) return_t;
  };

  _on1(Target target, Parm0 parm0) :
    m_target(target), m_parm0(parm0) {}

  template <typename Arg0>
  typename _<Arg0>::return_t operator () (Arg0& arg0) {
    return m_target(m_parm0(arg0));
  }

  template <typename Arg0>
  typename _<const Arg0>::return_t operator () (const Arg0& arg0) {
    return m_target(m_parm0(arg0));
  }

  Target m_target;
  Parm0  m_parm0;
};

}

#endif

