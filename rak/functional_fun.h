// rak - Rakshasa's toolbox
// Copyright (C) 2005-2006, Jari Sundell
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

// This file contains functors that wrap function points and member
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

namespace rak {

template <typename Result>
class function_base0 {
public:
  virtual ~function_base0() {}

  virtual Result operator () () = 0;
};

template <typename Result, typename Arg1>
class function_base1 {
public:
  virtual ~function_base1() {}

  virtual Result operator () (Arg1 arg1) = 0;
};

template <typename Result>
class function0 {
public:
  typedef Result                 result_type;
  typedef function_base0<Result> base_type;

  bool is_valid() const { return m_base.get() != NULL; }

  void set(base_type* base) { m_base = std::auto_ptr<base_type>(base); }

  Result operator () () { return (*m_base)(); }

private:
  std::auto_ptr<base_type> m_base;
};

template <typename Result, typename Arg1>
class function1 {
public:
  typedef Result                       result_type;
  typedef function_base1<Result, Arg1> base_type;

  bool is_valid() const { return m_base.get() != NULL; }

  void set(base_type* base) { m_base = std::auto_ptr<base_type>(base); }

  Result operator () (Arg1 arg1) { return (*m_base)(arg1); }

private:
  std::auto_ptr<base_type> m_base;
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

template <typename Object, typename Result>
function_base0<Result>*
mem_fn(Object* object, Result (Object::*func)()) {
  return new mem_fn0_t<Object, Result>(object, func);
}

template <typename Object, typename Result, typename Arg1>
function_base1<Result, Arg1>*
mem_fn(Object* object, Result (Object::*func)(Arg1)) {
  return new mem_fn1_t<Object, Result, Arg1>(object, func);
}

template <typename Object, typename Result>
function_base0<Result>*
mem_fn(const Object* object, Result (Object::*func)() const) {
  return new const_mem_fn0_t<Object, Result>(object, func);
}

template <typename Object, typename Result, typename Arg1>
function_base1<Result, Arg1>*
mem_fn(const Object* object, Result (Object::*func)(Arg1) const) {
  return new const_mem_fn1_t<Object, Result, Arg1>(object, func);
}

template <typename Object, typename Result, typename Arg1>
function_base0<Result>*
bind_mem_fn(Object* object, Result (Object::*func)(Arg1), const Arg1 arg1) {
  return new mem_fn0_b1_t<Object, Result, Arg1>(object, func, arg1);
}

}

#endif
