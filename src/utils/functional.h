#ifndef RTORRENT_FUNCTIONAL_H
#define RTORRENT_FUNCTIONAL_H

#include <functional>

namespace func {

template <typename Iterator, typename Ftor>
inline void for_each(Iterator first, Iterator last, Ftor ftor) {
  Iterator tmp;

  while (first != last) {
    tmp = first++;
    
    ftor(*tmp);
  }
}

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
