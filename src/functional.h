#ifndef RTORRENT_FUNCTIONAL_H
#define RTORRENT_FUNCTIONAL_H

#include <functional>

namespace func {

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

template <typename Dest, typename Src>
struct _on : public std::unary_function<typename Src::argument_type, typename Dest::result_type>  {

  _on(Dest d, Src s) : m_dest(d), m_src(s) {}

  typename Dest::result_type operator () (typename Src::argument_type arg) {
    return m_dest(m_src(arg));
  }

  Dest m_dest;
  Src m_src;
};
    
template <typename Dest, typename Src>
inline _on<Dest, Src>
on(Dest d, Src s) {
  return _on<Dest, Src>(d, s);
}  

struct _call_delete
  : public std::unary_function<void, void> {
  
  template <typename Type>
  void operator () (Type* t) {
    delete t;
  }
};

inline _call_delete
call_delete() {
  return _call_delete();
}

}

#endif
