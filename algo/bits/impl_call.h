// -*- c++ -*-
/* Do not edit! -- generated file */

#ifndef ALGO_BITS_IMPL_CALL_H
#define ALGO_BITS_IMPL_CALL_H

namespace algo {

template <typename Func, typename Ret>
struct _ptr_fun0 {
  
  template <typename Arg0, bool r = traits<Ret>::REF>
  struct _ {
    typedef Ret return_t;
  };

  template <typename Arg0>
  struct _<Arg0, false> {
    typedef const Ret return_t;
  };

  _ptr_fun0(Func func) :
    m_func(func) {}

  template <typename Arg0>
  typename _<Arg0&>::return_t operator () (Arg0& arg0) {
    return m_func();
  }

  template <typename Arg0>
  typename _<const Arg0&>::return_t operator () (const Arg0& arg0) {
    return m_func();
  }

  Func  m_func;

};

template <typename Func, typename Ret, typename Parm1>
struct _ptr_fun1 {
  
  template <typename Arg0, bool r = traits<Ret>::REF>
  struct _ {
    typedef Ret return_t;
  };

  template <typename Arg0>
  struct _<Arg0, false> {
    typedef const Ret return_t;
  };

  _ptr_fun1(Func func, Parm1 parm1) :
    m_func(func), m_parm1(parm1) {}

  template <typename Arg0>
  typename _<Arg0&>::return_t operator () (Arg0& arg0) {
    return m_func(m_parm1(arg0));
  }

  template <typename Arg0>
  typename _<const Arg0&>::return_t operator () (const Arg0& arg0) {
    return m_func(m_parm1(arg0));
  }

  Func  m_func;

  Parm1 m_parm1;
};

template <typename Func, typename Ret, typename Parm1,typename Parm2>
struct _ptr_fun2 {
  
  template <typename Arg0, bool r = traits<Ret>::REF>
  struct _ {
    typedef Ret return_t;
  };

  template <typename Arg0>
  struct _<Arg0, false> {
    typedef const Ret return_t;
  };

  _ptr_fun2(Func func, Parm1 parm1,Parm2 parm2) :
    m_func(func), m_parm1(parm1),m_parm2(parm2) {}

  template <typename Arg0>
  typename _<Arg0&>::return_t operator () (Arg0& arg0) {
    return m_func(m_parm1(arg0),m_parm2(arg0));
  }

  template <typename Arg0>
  typename _<const Arg0&>::return_t operator () (const Arg0& arg0) {
    return m_func(m_parm1(arg0),m_parm2(arg0));
  }

  Func  m_func;

  Parm1 m_parm1;
  Parm2 m_parm2;
};

template <typename Func, typename Ret, typename Parm1,typename Parm2,typename Parm3>
struct _ptr_fun3 {
  
  template <typename Arg0, bool r = traits<Ret>::REF>
  struct _ {
    typedef Ret return_t;
  };

  template <typename Arg0>
  struct _<Arg0, false> {
    typedef const Ret return_t;
  };

  _ptr_fun3(Func func, Parm1 parm1,Parm2 parm2,Parm3 parm3) :
    m_func(func), m_parm1(parm1),m_parm2(parm2),m_parm3(parm3) {}

  template <typename Arg0>
  typename _<Arg0&>::return_t operator () (Arg0& arg0) {
    return m_func(m_parm1(arg0),m_parm2(arg0),m_parm3(arg0));
  }

  template <typename Arg0>
  typename _<const Arg0&>::return_t operator () (const Arg0& arg0) {
    return m_func(m_parm1(arg0),m_parm2(arg0),m_parm3(arg0));
  }

  Func  m_func;

  Parm1 m_parm1;
  Parm2 m_parm2;
  Parm3 m_parm3;
};

template <typename Func, typename Ret, typename Parm1,typename Parm2,typename Parm3,typename Parm4>
struct _ptr_fun4 {
  
  template <typename Arg0, bool r = traits<Ret>::REF>
  struct _ {
    typedef Ret return_t;
  };

  template <typename Arg0>
  struct _<Arg0, false> {
    typedef const Ret return_t;
  };

  _ptr_fun4(Func func, Parm1 parm1,Parm2 parm2,Parm3 parm3,Parm4 parm4) :
    m_func(func), m_parm1(parm1),m_parm2(parm2),m_parm3(parm3),m_parm4(parm4) {}

  template <typename Arg0>
  typename _<Arg0&>::return_t operator () (Arg0& arg0) {
    return m_func(m_parm1(arg0),m_parm2(arg0),m_parm3(arg0),m_parm4(arg0));
  }

  template <typename Arg0>
  typename _<const Arg0&>::return_t operator () (const Arg0& arg0) {
    return m_func(m_parm1(arg0),m_parm2(arg0),m_parm3(arg0),m_parm4(arg0));
  }

  Func  m_func;

  Parm1 m_parm1;
  Parm2 m_parm2;
  Parm3 m_parm3;
  Parm4 m_parm4;
};


template <typename Target, typename Func, typename Ret>
struct _mem_fun0 {
  
  template <typename Arg0>
  struct _ {
    typedef Ret return_t;
  };

  _mem_fun0(Target target, Func func) :
    m_target(target), m_func(func) {}

  template <typename Arg0>
  typename _<Arg0&>::return_t operator () (Arg0& arg0) {
    return (m_target(arg0).*m_func)();
  }

  template <typename Arg0>
  typename _<const Arg0&>::return_t operator () (const Arg0& arg0) {
    return (m_target(arg0).*m_func)();
  }

  Target m_target;
  Func  m_func;

};

template <typename Target, typename Func, typename Ret, typename Parm1>
struct _mem_fun1 {
  
  template <typename Arg0>
  struct _ {
    typedef Ret return_t;
  };

  _mem_fun1(Target target, Func func, Parm1 parm1) :
    m_target(target), m_func(func), m_parm1(parm1) {}

  template <typename Arg0>
  typename _<Arg0&>::return_t operator () (Arg0& arg0) {
    return (m_target(arg0).*m_func)(m_parm1(arg0));
  }

  template <typename Arg0>
  typename _<const Arg0&>::return_t operator () (const Arg0& arg0) {
    return (m_target(arg0).*m_func)(m_parm1(arg0));
  }

  Target m_target;
  Func  m_func;

  Parm1 m_parm1;
};

template <typename Target, typename Func, typename Ret, typename Parm1,typename Parm2>
struct _mem_fun2 {
  
  template <typename Arg0>
  struct _ {
    typedef Ret return_t;
  };

  _mem_fun2(Target target, Func func, Parm1 parm1,Parm2 parm2) :
    m_target(target), m_func(func), m_parm1(parm1),m_parm2(parm2) {}

  template <typename Arg0>
  typename _<Arg0&>::return_t operator () (Arg0& arg0) {
    return (m_target(arg0).*m_func)(m_parm1(arg0),m_parm2(arg0));
  }

  template <typename Arg0>
  typename _<const Arg0&>::return_t operator () (const Arg0& arg0) {
    return (m_target(arg0).*m_func)(m_parm1(arg0),m_parm2(arg0));
  }

  Target m_target;
  Func  m_func;

  Parm1 m_parm1;
  Parm2 m_parm2;
};

template <typename Target, typename Func, typename Ret, typename Parm1,typename Parm2,typename Parm3>
struct _mem_fun3 {
  
  template <typename Arg0>
  struct _ {
    typedef Ret return_t;
  };

  _mem_fun3(Target target, Func func, Parm1 parm1,Parm2 parm2,Parm3 parm3) :
    m_target(target), m_func(func), m_parm1(parm1),m_parm2(parm2),m_parm3(parm3) {}

  template <typename Arg0>
  typename _<Arg0&>::return_t operator () (Arg0& arg0) {
    return (m_target(arg0).*m_func)(m_parm1(arg0),m_parm2(arg0),m_parm3(arg0));
  }

  template <typename Arg0>
  typename _<const Arg0&>::return_t operator () (const Arg0& arg0) {
    return (m_target(arg0).*m_func)(m_parm1(arg0),m_parm2(arg0),m_parm3(arg0));
  }

  Target m_target;
  Func  m_func;

  Parm1 m_parm1;
  Parm2 m_parm2;
  Parm3 m_parm3;
};

template <typename Target, typename Func, typename Ret, typename Parm1,typename Parm2,typename Parm3,typename Parm4>
struct _mem_fun4 {
  
  template <typename Arg0>
  struct _ {
    typedef Ret return_t;
  };

  _mem_fun4(Target target, Func func, Parm1 parm1,Parm2 parm2,Parm3 parm3,Parm4 parm4) :
    m_target(target), m_func(func), m_parm1(parm1),m_parm2(parm2),m_parm3(parm3),m_parm4(parm4) {}

  template <typename Arg0>
  typename _<Arg0&>::return_t operator () (Arg0& arg0) {
    return (m_target(arg0).*m_func)(m_parm1(arg0),m_parm2(arg0),m_parm3(arg0),m_parm4(arg0));
  }

  template <typename Arg0>
  typename _<const Arg0&>::return_t operator () (const Arg0& arg0) {
    return (m_target(arg0).*m_func)(m_parm1(arg0),m_parm2(arg0),m_parm3(arg0),m_parm4(arg0));
  }

  Target m_target;
  Func  m_func;

  Parm1 m_parm1;
  Parm2 m_parm2;
  Parm3 m_parm3;
  Parm4 m_parm4;
};



}

#endif
