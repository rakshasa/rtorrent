#ifndef ALGO_BITS_IMPL_LAMBDA_H
#define ALGO_BITS_IMPL_LAMBDA_H

#define ALGO_OP_BINARY( NAME, OP )							\
template <typename Ftor1, typename Ftor2>						\
struct _op_##NAME {									\
											\
  template <typename Arg0>								\
  struct _ {										\
    typedef typename tmpl_if<traits<Arg0>::REF && !traits<Arg0>::CONST,			\
                             Arg0, typename traits<Arg0>::b_ref_t>::result Type;	\
											\
    typedef typeof(_makeT<Ftor1>()(_makeT<Type>()) OP					\
		   _makeT<Ftor2>()(_makeT<Type>())) return_t;				\
  };											\
											\
  _op_##NAME(Ftor1 ftor1, Ftor2 ftor2) : m_ftor1(ftor1), m_ftor2(ftor2) {}		\
											\
  template <typename Arg0>								\
  typename _<Arg0&>::return_t operator () (Arg0& arg0) {				\
    return m_ftor1(arg0) OP m_ftor2(arg0);						\
  }											\
											\
  template <typename Arg0>								\
  typename _<const Arg0&>::return_t operator () (const Arg0& arg0) {			\
    return m_ftor1(arg0) OP m_ftor2(arg0);						\
  }											\
											\
  Ftor1 m_ftor1;									\
  Ftor2 m_ftor2;									\
};

#define ALGO_MK_BINARY( NAME, OP )					\
template <typename Ftor1, typename Ftor2>				\
inline _wrapper<_op_##NAME<Ftor1, Ftor2> >				\
operator OP (_wrapper<Ftor1> w1, _wrapper<Ftor2> w2) {			\
  return make_wrapper(_op_##NAME<Ftor1, Ftor2>(w1.m_ftor, w2.m_ftor));	\
}

#define ALGO_OP_UNARY( NAME, OP )							\
template <typename Ftor1>								\
struct _op_##NAME {									\
											\
  template <typename Arg0>								\
  struct _ {										\
    typedef typename tmpl_if<traits<Arg0>::REF && !traits<Arg0>::CONST,			\
                             Arg0, typename traits<Arg0>::b_ref_t>::result Type;	\
											\
    typedef typeof( OP _makeT<Ftor1>()(_makeT<Type>()) ) return_t;			\
  };											\
											\
  _op_##NAME(Ftor1 ftor1) : m_ftor1(ftor1) {}						\
											\
  template <typename Arg0>								\
  typename _<Arg0&>::return_t operator () (Arg0& arg0) {				\
    return OP m_ftor1(arg0);								\
  }											\
											\
  template <typename Arg0>								\
  typename _<const Arg0&>::return_t operator () (const Arg0& arg0) {			\
    return OP m_ftor1(arg0);								\
  }											\
											\
  Ftor1 m_ftor1;									\
};											\
											\
template <typename Ftor1>								\
inline _wrapper<_op_##NAME<Ftor1> >							\
operator OP (_wrapper<Ftor1> w1) {							\
  return make_wrapper(_op_##NAME<Ftor1>(w1.m_ftor));					\
}

namespace algo {

template <typename Ftor, typename Ret>
struct op_cast {
  
  template <typename Arg0>
  struct _ {
    typedef Ret return_t;
  };

  op_cast(Ftor ftor) : m_ftor(ftor) {}

  template <typename Arg0>
  typename _<Arg0&>::return_t operator () (Arg0& arg0) {
    return m_ftor(arg0);
  }

  template <typename Arg0>
  typename _<const Arg0&>::return_t operator () (const Arg0& arg0) {
    return m_ftor(arg0);
  }

  Ftor m_ftor;
};  

}

#endif
