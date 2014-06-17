// rak - Rakshasa's toolbox
// Copyright (C) 2005-2007, Jari Sundell
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
// In addition, as a special exception, the copyright holders give
// permission to link the code of portions of this program with the
// OpenSSL library under certain conditions as described in each
// individual source file, and distribute linked combinations
// including the two.
//
// You must obey the GNU General Public License in all respects for
// all of the code used other than OpenSSL.  If you modify file(s)
// with this exception, you may extend this exception to your version
// of the file(s), but you are not obligated to do so.  If you do not
// wish to do so, delete this exception statement from your version.
// If you delete this exception statement from all source files in the
// program, then also delete it here.
//
// Contact:  Jari Sundell <jaris@ifi.uio.no>
//
//           Skomakerveien 33
//           3185 Skoppum, NORWAY

// This file contains functors that wrap function pointers and member
// function pointers.
//
// 'fn' functors are polymorphic and derives from 'rak::function' and
// thus is less strict about types, this adds the cost of calling a
// virtual function.
//
// 'fun' functors are non-polymorphic and thus cheaper, but requires
// the target object's type in the functor's template arguments.
//
// This should be replaced with TR1 stuff when it becomes widely
// available. At the moment it behaves like std::auto_ptr, so be
// careful when copying.

#ifndef RAK_FUNCTIONAL_FUN_H
#define RAK_FUNCTIONAL_FUN_H

#include <memory>
#include <functional>
#include lt_tr1_functional
#include lt_tr1_memory

namespace rak {

template <typename Result>
class function_base0 {
public:
  virtual ~function_base0() {}

  virtual Result operator () () = 0;
};

template <typename Result, typename Arg1>
class function_base1 : public std::unary_function<Arg1, Result> {
public:
  virtual ~function_base1() {}

  virtual Result operator () (Arg1 arg1) = 0;
};

template <typename Result, typename Arg1, typename Arg2>
class function_base2 : public std::binary_function<Arg1, Arg2, Result> {
public:
  virtual ~function_base2() {}

  virtual Result operator () (Arg1 arg1, Arg2 arg2) = 0;
};

template <typename Result, typename Arg1, typename Arg2, typename Arg3>
class function_base3 {
public:
  virtual ~function_base3() {}

  virtual Result operator () (Arg1 arg1, Arg2 arg2, Arg3 arg3) = 0;
};

template <typename Result>
class function0 {
public:
  typedef Result                 result_type;
  typedef function_base0<Result> base_type;

  bool                is_valid() const     { return m_base.get() != NULL; }

  void                set(base_type* base) { m_base = std::shared_ptr<base_type>(base); }
  base_type*          release()            { return m_base.release(); }

  Result operator () ()                    { return (*m_base)(); }

private:
  std::shared_ptr<base_type> m_base;
};

template <typename Result, typename Arg1>
class function1 {
public:
  typedef Result                       result_type;
  typedef function_base1<Result, Arg1> base_type;

  bool                is_valid() const     { return m_base.get() != NULL; }

  void                set(base_type* base) { m_base = std::shared_ptr<base_type>(base); }
  base_type*          release()            { return m_base.release(); }

  Result operator () (Arg1 arg1)           { return (*m_base)(arg1); }

private:
  std::shared_ptr<base_type> m_base;
};

template <typename Result, typename Arg1, typename Arg2>
class function2 {
public:
  typedef Result                             result_type;
  typedef function_base2<Result, Arg1, Arg2> base_type;

  bool                is_valid() const     { return m_base.get() != NULL; }

  void                set(base_type* base) { m_base = std::shared_ptr<base_type>(base); }
  base_type*          release()            { return m_base.release(); }

  Result operator () (Arg1 arg1, Arg2 arg2) { return (*m_base)(arg1, arg2); }

private:
  std::shared_ptr<base_type> m_base;
};

template <typename Result, typename Arg2>
class function2<Result, void, Arg2> {
public:
  typedef Result                             result_type;
  typedef function_base1<Result, Arg2>       base_type;

  bool                is_valid() const     { return m_base.get() != NULL; }

  void                set(base_type* base) { m_base = std::shared_ptr<base_type>(base); }
  base_type*          release()            { return m_base.release(); }

  Result operator () (Arg2 arg2)           { return (*m_base)(arg2); }

  template <typename Discard>
  Result operator () (Discard discard, Arg2 arg2) { return (*m_base)(arg2); }

private:
  std::shared_ptr<base_type> m_base;
};

template <typename Result, typename Arg1, typename Arg2, typename Arg3>
class function3 {
public:
  typedef Result                                   result_type;
  typedef function_base3<Result, Arg1, Arg2, Arg3> base_type;

  bool                is_valid() const     { return m_base.get() != NULL; }

  void                set(base_type* base) { m_base = std::shared_ptr<base_type>(base); }
  base_type*          release()            { return m_base.release(); }

  Result operator () (Arg1 arg1, Arg2 arg2, Arg3 arg3) { return (*m_base)(arg1, arg2, arg3); }

private:
  std::shared_ptr<base_type> m_base;
};

template <typename Result>
class ptr_fn0_t : public function_base0<Result> {
public:
  typedef Result (*Func)();

  ptr_fn0_t(Func func) : m_func(func) {}
  virtual ~ptr_fn0_t() {}
  
  virtual Result operator () () { return m_func(); }

private:
  Func    m_func;
};

template <typename Result, typename Arg1>
class ptr_fn1_t : public function_base1<Result, Arg1> {
public:
  typedef Result (*Func)(Arg1);

  ptr_fn1_t(Func func) : m_func(func) {}
  virtual ~ptr_fn1_t() {}
  
  virtual Result operator () (Arg1 arg1) { return m_func(arg1); }

private:
  Func    m_func;
};

template <typename Result, typename Arg1, typename Arg2>
class ptr_fn2_t : public function_base2<Result, Arg1, Arg2> {
public:
  typedef Result (*Func)(Arg1, Arg2);

  ptr_fn2_t(Func func) : m_func(func) {}
  virtual ~ptr_fn2_t() {}
  
  virtual Result operator () (Arg1 arg1, Arg2 arg2) { return m_func(arg1, arg2); }

private:
  Func    m_func;
};

template <typename Object, typename Result>
class mem_fn0_t : public function_base0<Result> {
public:
  typedef Result (Object::*Func)();

  mem_fn0_t(Object* object, Func func) : m_object(object), m_func(func) {}
  virtual ~mem_fn0_t() {}
  
  virtual Result operator () () { return (m_object->*m_func)(); }

private:
  Object* m_object;
  Func    m_func;
};

template <typename Object, typename Result, typename Arg1>
class mem_fn1_t : public function_base1<Result, Arg1> {
public:
  typedef Result (Object::*Func)(Arg1);

  mem_fn1_t(Object* object, Func func) : m_object(object), m_func(func) {}
  virtual ~mem_fn1_t() {}
  
  virtual Result operator () (Arg1 arg1) { return (m_object->*m_func)(arg1); }

private:
  Object* m_object;
  Func    m_func;
};

template <typename Object, typename Result, typename Arg1, typename Arg2, typename Arg3>
class mem_fn3_t : public function_base3<Result, Arg1, Arg2, Arg3> {
public:
  typedef Result (Object::*Func)(Arg1, Arg2, Arg3);

  mem_fn3_t(Object* object, Func func) : m_object(object), m_func(func) {}
  virtual ~mem_fn3_t() {}
  
  virtual Result operator () (Arg1 arg1, Arg2 arg2, Arg3 arg3) { return (m_object->*m_func)(arg1, arg2, arg3); }

private:
  Object* m_object;
  Func    m_func;
};

template <typename Object, typename Result, typename Arg1, typename Arg2>
class mem_fn2_t : public function_base2<Result, Arg1, Arg2> {
public:
  typedef Result (Object::*Func)(Arg1, Arg2);

  mem_fn2_t(Object* object, Func func) : m_object(object), m_func(func) {}
  virtual ~mem_fn2_t() {}
  
  virtual Result operator () (Arg1 arg1, Arg2 arg2) { return (m_object->*m_func)(arg1, arg2); }

private:
  Object* m_object;
  Func    m_func;
};

template <typename Object, typename Result>
class const_mem_fn0_t : public function_base0<Result> {
public:
  typedef Result (Object::*Func)() const;

  const_mem_fn0_t(const Object* object, Func func) : m_object(object), m_func(func) {}
  virtual ~const_mem_fn0_t() {}
  
  virtual Result operator () () { return (m_object->*m_func)(); }

private:
  const Object* m_object;
  Func          m_func;
};

template <typename Object, typename Result, typename Arg1>
class const_mem_fn1_t : public function_base1<Result, Arg1> {
public:
  typedef Result (Object::*Func)(Arg1) const;

  const_mem_fn1_t(const Object* object, Func func) : m_object(object), m_func(func) {}
  virtual ~const_mem_fn1_t() {}
  
  virtual Result operator () (Arg1 arg1) { return (m_object->*m_func)(arg1); }

private:
  const Object* m_object;
  Func          m_func;
};

// Unary functor with a bound argument.
template <typename Object, typename Result, typename Arg1>
class mem_fn0_b1_t : public function_base0<Result> {
public:
  typedef Result (Object::*Func)(Arg1);

  mem_fn0_b1_t(Object* object, Func func, const Arg1 arg1) : m_object(object), m_func(func), m_arg1(arg1) {}
  virtual ~mem_fn0_b1_t() {}
  
  virtual Result operator () () { return (m_object->*m_func)(m_arg1); }

private:
  Object*    m_object;
  Func       m_func;
  const Arg1 m_arg1;
};

template <typename Object, typename Result, typename Arg1, typename Arg2>
class mem_fn1_b1_t : public function_base1<Result, Arg2> {
public:
  typedef Result (Object::*Func)(Arg1, Arg2);

  mem_fn1_b1_t(Object* object, Func func, const Arg1 arg1) : m_object(object), m_func(func), m_arg1(arg1) {}
  virtual ~mem_fn1_b1_t() {}
  
  virtual Result operator () (const Arg2 arg2) { return (m_object->*m_func)(m_arg1, arg2); }

private:
  Object*    m_object;
  Func       m_func;
  const Arg1 m_arg1;
};

template <typename Object, typename Result, typename Arg1, typename Arg2>
class mem_fn1_b2_t : public function_base1<Result, Arg1> {
public:
  typedef Result (Object::*Func)(Arg1, Arg2);

  mem_fn1_b2_t(Object* object, Func func, const Arg2 arg2) : m_object(object), m_func(func), m_arg2(arg2) {}
  virtual ~mem_fn1_b2_t() {}
  
  virtual Result operator () (const Arg1 arg1) { return (m_object->*m_func)(arg1, m_arg2); }

private:
  Object*    m_object;
  Func       m_func;
  const Arg2 m_arg2;
};

template <typename Result, typename Arg1>
class ptr_fn0_b1_t : public function_base0<Result> {
public:
  typedef Result (*Func)(Arg1);

  ptr_fn0_b1_t(Func func, const Arg1 arg1) : m_func(func), m_arg1(arg1) {}
  virtual ~ptr_fn0_b1_t() {}
  
  virtual Result operator () () { return m_func(m_arg1); }

private:
  Func    m_func;
  Arg1    m_arg1;
};

template <typename Result, typename Arg1, typename Arg2>
class ptr_fn1_b1_t : public function_base1<Result, Arg2> {
public:
  typedef Result (*Func)(Arg1, Arg2);

  ptr_fn1_b1_t(Func func, const Arg1 arg1) : m_func(func), m_arg1(arg1) {}
  virtual ~ptr_fn1_b1_t() {}
  
  virtual Result operator () (Arg2 arg2) { return m_func(m_arg1, arg2); }

private:
  Func    m_func;
  Arg1    m_arg1;
};

template <typename Result, typename Arg1, typename Arg2, typename Arg3>
class ptr_fn2_b1_t : public function_base2<Result, Arg2, Arg3> {
public:
  typedef Result (*Func)(Arg1, Arg2, Arg3);

  ptr_fn2_b1_t(Func func, const Arg1 arg1) : m_func(func), m_arg1(arg1) {}
  virtual ~ptr_fn2_b1_t() {}
  
  virtual Result operator () (Arg2 arg2, Arg3 arg3) { return m_func(m_arg1, arg2, arg3); }

private:
  Func    m_func;
  Arg1    m_arg1;
};

template <typename Ftor>
class ftor_fn1_t : public function_base1<typename Ftor::result_type, typename Ftor::argument_type> {
public:
  typedef typename Ftor::result_type result_type;
  typedef typename Ftor::argument_type argument_type;

  ftor_fn1_t(Ftor ftor) : m_ftor(ftor) {}
  virtual ~ftor_fn1_t() {}
  
  virtual result_type operator () (argument_type arg1) { return m_ftor(arg1); }

private:
  Ftor    m_ftor;
};

template <typename Ftor>
class ftor_fn2_t : public function_base2<typename Ftor::result_type, typename Ftor::first_argument_type, typename Ftor::second_argument_type> {
public:
  typedef typename Ftor::result_type result_type;
  typedef typename Ftor::first_argument_type first_argument_type;
  typedef typename Ftor::second_argument_type second_argument_type;

  ftor_fn2_t(Ftor ftor) : m_ftor(ftor) {}
  virtual ~ftor_fn2_t() {}
  
  virtual result_type operator () (first_argument_type arg1, second_argument_type arg2) { return m_ftor(arg1, arg2); }

private:
  Ftor    m_ftor;
};

template <typename Result>
class value_fn0_t : public function_base0<Result> {
public:
  value_fn0_t(const Result& val) : m_value(val) {}
  
  virtual Result operator () () { return m_value; }

private:
  Result  m_value;
};

template <typename Result, typename SrcResult>
class convert_fn0_t : public function_base0<Result> {
public:
  typedef function0<SrcResult> src_type;

  convert_fn0_t(typename src_type::base_type* object) { m_object.set(object); }
  virtual ~convert_fn0_t() {}
  
  virtual Result operator () () {
    return m_object();
  }

private:
  src_type m_object;
};

template <typename Result, typename Arg1, typename SrcResult, typename SrcArg1>
class convert_fn1_t : public function_base1<Result, Arg1> {
public:
  typedef function1<SrcResult, SrcArg1> src_type;

  convert_fn1_t(typename src_type::base_type* object) { m_object.set(object); }
  virtual ~convert_fn1_t() {}
  
  virtual Result operator () (Arg1 arg1) {
    return m_object(arg1);
  }

private:
  src_type m_object;
};

template <typename Result, typename Arg1, typename Arg2, typename SrcResult, typename SrcArg1, typename SrcArg2>
class convert_fn2_t : public function_base2<Result, Arg1, Arg2> {
public:
  typedef function2<SrcResult, SrcArg1, SrcArg2> src_type;

  convert_fn2_t(typename src_type::base_type* object) { m_object.set(object); }
  virtual ~convert_fn2_t() {}
  
  virtual Result operator () (Arg1 arg1, Arg2 arg2) {
    return m_object(arg1, arg2);
  }

private:
  src_type m_object;
};

template <typename Result>
inline function_base0<Result>*
ptr_fn(Result (*func)()) {
  return new ptr_fn0_t<Result>(func);
}

template <typename Arg1, typename Result>
inline function_base1<Result, Arg1>*
ptr_fn(Result (*func)(Arg1)) {
  return new ptr_fn1_t<Result, Arg1>(func);
}

template <typename Arg1, typename Arg2, typename Result>
inline function_base2<Result, Arg1, Arg2>*
ptr_fn(Result (*func)(Arg1, Arg2)) {
  return new ptr_fn2_t<Result, Arg1, Arg2>(func);
}

template <typename Result, typename Object>
inline function_base0<Result>*
mem_fn(Object* object, Result (Object::*func)()) {
  return new mem_fn0_t<Object, Result>(object, func);
}

template <typename Arg1, typename Result, typename Object>
inline function_base1<Result, Arg1>*
mem_fn(Object* object, Result (Object::*func)(Arg1)) {
  return new mem_fn1_t<Object, Result, Arg1>(object, func);
}

template <typename Arg1, typename Arg2, typename Result, typename Object>
inline function_base2<Result, Arg1, Arg2>*
mem_fn(Object* object, Result (Object::*func)(Arg1, Arg2)) {
  return new mem_fn2_t<Object, Result, Arg1, Arg2>(object, func);
}

template <typename Arg1, typename Arg2, typename Arg3, typename Result, typename Object>
inline function_base3<Result, Arg1, Arg2, Arg3>*
mem_fn(Object* object, Result (Object::*func)(Arg1, Arg2, Arg3)) {
  return new mem_fn3_t<Object, Result, Arg1, Arg2, Arg3>(object, func);
}

template <typename Result, typename Object>
inline function_base0<Result>*
mem_fn(const Object* object, Result (Object::*func)() const) {
  return new const_mem_fn0_t<Object, Result>(object, func);
}

template <typename Arg1, typename Result, typename Object>
inline function_base1<Result, Arg1>*
mem_fn(const Object* object, Result (Object::*func)(Arg1) const) {
  return new const_mem_fn1_t<Object, Result, Arg1>(object, func);
}

template <typename Arg1, typename Result, typename Object>
inline function_base0<Result>*
bind_mem_fn(Object* object, Result (Object::*func)(Arg1), const Arg1 arg1) {
  return new mem_fn0_b1_t<Object, Result, Arg1>(object, func, arg1);
}

template <typename Arg1, typename Arg2, typename Result, typename Object>
inline function_base1<Result, Arg2>*
bind_mem_fn(Object* object, Result (Object::*func)(Arg1, Arg2), const Arg1 arg1) {
  return new mem_fn1_b1_t<Object, Result, Arg1, Arg2>(object, func, arg1);
}

template <typename Arg1, typename Arg2, typename Result, typename Object>
inline function_base1<Result, Arg1>*
bind2_mem_fn(Object* object, Result (Object::*func)(Arg1, Arg2), const Arg2 arg2) {
  return new mem_fn1_b2_t<Object, Result, Arg1, Arg2>(object, func, arg2);
}

template <typename Arg1, typename Result>
inline function_base0<Result>*
bind_ptr_fn(Result (*func)(Arg1), const Arg1 arg1) {
  return new ptr_fn0_b1_t<Result, Arg1>(func, arg1);
}

template <typename Arg1, typename Arg2, typename Result>
inline function_base1<Result, Arg2>*
bind_ptr_fn(Result (*func)(Arg1, Arg2), const Arg1 arg1) {
  return new ptr_fn1_b1_t<Result, Arg1, Arg2>(func, arg1);
}

template <typename Arg1, typename Arg2, typename Arg3, typename Result>
inline function_base2<Result, Arg2, Arg3>*
bind_ptr_fn(Result (*func)(Arg1, Arg2, Arg3), const Arg1 arg1) {
  return new ptr_fn2_b1_t<Result, Arg1, Arg2, Arg3>(func, arg1);
}

template <typename Ftor>
inline function_base1<typename Ftor::result_type, typename Ftor::argument_type>*
ftor_fn1(Ftor ftor) {
  return new ftor_fn1_t<Ftor>(ftor);
}

template <typename Ftor>
inline function_base2<typename Ftor::result_type, typename Ftor::first_argument_type, typename Ftor::second_argument_type>*
ftor_fn2(Ftor ftor) {
  return new ftor_fn2_t<Ftor>(ftor);
}

template <typename Result>
inline function_base0<Result>*
value_fn(const Result& val) {
  return new value_fn0_t<Result>(val);
}

template <typename A, typename B>
struct equal_types_t {
  typedef A first_type;
  typedef B second_type;

  const static int result = 0;
};

template <typename A>
struct equal_types_t<A, A> {
  typedef A first_type;
  typedef A second_type;

  const static int result = 1;
};

template <typename Result, typename SrcResult>
inline function_base0<Result>*
convert_fn(function_base0<SrcResult>* src) {
  if (equal_types_t<function_base0<Result>, function_base0<SrcResult> >::result)
    // The pointer cast never gets done if the types are different,
    // but needs to be here to pleasant the compiler.
    return reinterpret_cast<typename equal_types_t<function_base0<Result>, function_base0<SrcResult> >::first_type*>(src);
  else
    return new convert_fn0_t<Result, SrcResult>(src);
}

template <typename Result, typename Arg1, typename SrcResult, typename SrcArg1>
inline function_base1<Result, Arg1>*
convert_fn(function_base1<SrcResult, SrcArg1>* src) {
  if (equal_types_t<function_base1<Result, Arg1>, function_base1<SrcResult, SrcArg1> >::result)
    // The pointer cast never gets done if the types are different,
    // but needs to be here to pleasant the compiler.
    return reinterpret_cast<typename equal_types_t<function_base1<Result, Arg1>, function_base1<SrcResult, SrcArg1> >::first_type*>(src);
  else
    return new convert_fn1_t<Result, Arg1, SrcResult, SrcArg1>(src);
}

template <typename Result, typename Arg1, typename Arg2, typename SrcResult, typename SrcArg1, typename SrcArg2>
inline function_base2<Result, Arg1, Arg2>*
convert_fn(function_base2<SrcResult, SrcArg1, SrcArg2>* src) {
  if (equal_types_t<function_base2<Result, Arg1, Arg2>, function_base2<SrcResult, SrcArg1, SrcArg2> >::result)
    // The pointer cast never gets done if the types are different,
    // but needs to be here to pleasant the compiler.
    return reinterpret_cast<typename equal_types_t<function_base2<Result, Arg1, Arg2>, function_base2<SrcResult, SrcArg1, SrcArg2> >::first_type*>(src);
  else
    return new convert_fn2_t<Result, Arg1, Arg2, SrcResult, SrcArg1, SrcArg2>(src);
}

}

#endif
