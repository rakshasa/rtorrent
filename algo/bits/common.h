#ifndef ALGO_COMMON_H
#define ALGO_COMMON_H

namespace algo {

struct _nil {
};

struct _one {
  char c[1];
};

struct _two {
  char c[2];
};

template <bool C, typename Then, typename Else>
struct tmpl_if;

template <typename Then, typename Else>
struct tmpl_if<true, Then, Else> {
  typedef Then result;
};

template <typename Then, typename Else>
struct tmpl_if<false, Then, Else> {
  typedef Else result;
};

template <typename T>
struct traits {
  enum {
    REF = false,
    CONST = false
  };

  typedef T        orig_t;
  typedef T        type_t;
  typedef T&       ref_t;

  typedef T        b_orig_t;
  typedef T        b_type_t;
  typedef T&       b_ref_t;

  typedef const T  c_orig_t;
  typedef const T  c_type_t;
  typedef const T& c_ref_t;
};

template <typename T>
struct traits<T&> {
  enum {
    REF = true,
    CONST = false
  };

  typedef T&       orig_t;
  typedef T        type_t;
  typedef T&       ref_t;

  typedef T&       b_orig_t;
  typedef T        b_type_t;
  typedef T&       b_ref_t;

  typedef const T& c_orig_t;
  typedef const T  c_type_t;
  typedef const T& c_ref_t;
};

template <typename T>
struct traits<const T> {
  enum {
    REF = false,
    CONST = true
  };

  typedef const T  orig_t;
  typedef const T  type_t;
  typedef const T& ref_t;

  typedef T        b_orig_t;
  typedef T        b_type_t;
  typedef T&       b_ref_t;

  typedef const T  c_orig_t;
  typedef const T  c_type_t;
  typedef const T& c_ref_t;
};

template <typename T>
struct traits<const T&> {
  enum {
    REF = true,
    CONST = true
  };

  typedef const T& orig_t;
  typedef const T  type_t;
  typedef const T& ref_t;

  typedef T&       b_orig_t;
  typedef T        b_type_t;
  typedef T&       b_ref_t;

  typedef const T& c_orig_t;
  typedef const T  c_type_t;
  typedef const T& c_ref_t;
};

// Helper function used to generate an instantated type for typeof
// calls.
template <typename T>
T _makeT();

template <typename Ftor1, typename Ftor2>
struct _op_assign;

// This class acts like a wrapper identifying classes that
// are compatible with the algo structure.
template <typename Ftor>
struct _wrapper {
  
  template <typename Arg0>							
  struct _ {									
    typedef typeof(_makeT<Ftor>()(_makeT<Arg0>())) return_t;			
  };											

  _wrapper(Ftor ftor) : m_ftor(ftor) {}

  template <typename Arg0>
  typename _<Arg0&>::return_t operator () (Arg0& arg0 = _nil()) {
    return m_ftor(arg0);
  }

//   template <typename Arg0>
//   typename _<Arg0&>::return_t operator () (const Arg0& arg0) {
//     return m_ftor(arg0);
//   }

  template <typename Ftor2>
  inline _wrapper<_op_assign<Ftor, Ftor2> >
  operator = (_wrapper<Ftor2> w) {
    return make_wrapper(_op_assign<Ftor, Ftor2>(m_ftor, w.m_ftor));
  }

  Ftor m_ftor;
};

template <typename Ftor>
inline _wrapper<Ftor>
make_wrapper(Ftor ftor) {
  return _wrapper<Ftor>(ftor);
}

}

#endif
