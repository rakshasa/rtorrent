// -*- c++ -*-
/* Do not edit! -- generated file */

#ifndef ALGO_BITS_IMPL_MEMORY_H
#define ALGO_BITS_IMPL_MEMORY_H

namespace algo {

template <typename Ftor>
struct _delete {
  
  template <typename Arg0>
  struct _ {
    typedef void return_t;
  };
  
  _delete(Ftor ftor) : m_ftor(ftor) {}

  template <typename Arg0>
  typename _<Arg0&>::return_t operator () (Arg0& arg0) {
    delete m_ftor(arg0);
  }

  template <typename Arg0>
  typename _<const Arg0&>::return_t operator () (const Arg0& arg0) {
    delete m_ftor(arg0);
  }

  Ftor m_ftor;
};

template <typename Type>
struct _new0 {
  
  template <typename Arg0>
  struct _ {
    typedef Type* return_t;
  };

  template <typename Arg0>
  typename _<Arg0&>::return_t operator () (Arg0& arg0) {
    return new Type;
  }

  template <typename Arg0>
  typename _<const Arg0&>::return_t operator () (const Arg0& arg0) {
    return new Type;
  }
};

template <typename Type, typename P1>
struct _new1 {
  
  template <typename Arg0>
  struct _ {
    typedef Type* return_t;
  };

  _new1(P1 p1) :
    m_p1(p1) {}

  template <typename Arg0>
  typename _<Arg0&>::return_t operator () (Arg0& arg0) {
    return new Type(m_p1(arg0));
  }

  template <typename Arg0>
  typename _<const Arg0&>::return_t operator () (const Arg0& arg0) {
    return new Type(m_p1(arg0));
  }

  P1 m_p1;
};

template <typename Type, typename P1,typename P2>
struct _new2 {
  
  template <typename Arg0>
  struct _ {
    typedef Type* return_t;
  };

  _new2(P1 p1,P2 p2) :
    m_p1(p1),m_p2(p2) {}

  template <typename Arg0>
  typename _<Arg0&>::return_t operator () (Arg0& arg0) {
    return new Type(m_p1(arg0),m_p2(arg0));
  }

  template <typename Arg0>
  typename _<const Arg0&>::return_t operator () (const Arg0& arg0) {
    return new Type(m_p1(arg0),m_p2(arg0));
  }

  P1 m_p1;
  P2 m_p2;
};

template <typename Type, typename P1,typename P2,typename P3>
struct _new3 {
  
  template <typename Arg0>
  struct _ {
    typedef Type* return_t;
  };

  _new3(P1 p1,P2 p2,P3 p3) :
    m_p1(p1),m_p2(p2),m_p3(p3) {}

  template <typename Arg0>
  typename _<Arg0&>::return_t operator () (Arg0& arg0) {
    return new Type(m_p1(arg0),m_p2(arg0),m_p3(arg0));
  }

  template <typename Arg0>
  typename _<const Arg0&>::return_t operator () (const Arg0& arg0) {
    return new Type(m_p1(arg0),m_p2(arg0),m_p3(arg0));
  }

  P1 m_p1;
  P2 m_p2;
  P3 m_p3;
};

template <typename Type, typename P1,typename P2,typename P3,typename P4>
struct _new4 {
  
  template <typename Arg0>
  struct _ {
    typedef Type* return_t;
  };

  _new4(P1 p1,P2 p2,P3 p3,P4 p4) :
    m_p1(p1),m_p2(p2),m_p3(p3),m_p4(p4) {}

  template <typename Arg0>
  typename _<Arg0&>::return_t operator () (Arg0& arg0) {
    return new Type(m_p1(arg0),m_p2(arg0),m_p3(arg0),m_p4(arg0));
  }

  template <typename Arg0>
  typename _<const Arg0&>::return_t operator () (const Arg0& arg0) {
    return new Type(m_p1(arg0),m_p2(arg0),m_p3(arg0),m_p4(arg0));
  }

  P1 m_p1;
  P2 m_p2;
  P3 m_p3;
  P4 m_p4;
};


}

#endif
