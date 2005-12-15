// rak - Rakshasa's toolbox
// Copyright (C) 2005, Jari Sundell
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
class function_base {
public:
  virtual ~function_base() {}

  virtual Result operator () () = 0;
};

template <typename Result>
class function {
public:
  typedef Result result_type;
  typedef function_base<Result> Base;

  bool is_valid() const { return m_base.get() != NULL; }

  void set(Base* base) { m_base = std::auto_ptr<Base>(base); }

  Result operator () () { return (*m_base)(); }

private:
  std::auto_ptr<Base> m_base;
};

template <typename Object, typename Result>
class _mem_fn0 : public function_base<Result> {
public:
  typedef Result (Object::*Func)();

  _mem_fn0(Object* object, Func func) : m_object(object), m_func(func) {}
  virtual ~_mem_fn0() {}
  
  virtual Result operator () () { return (m_object->*m_func)(); }

private:
  Object* m_object;
  Func    m_func;
};

template <typename Object, typename Result>
class _const_mem_fn0 : public function_base<Result> {
public:
  typedef Result (Object::*Func)() const;

  _const_mem_fn0(const Object* object, Func func) : m_object(object), m_func(func) {}
  virtual ~_const_mem_fn0() {}
  
  virtual Result operator () () { return (m_object->*m_func)(); }

private:
  const Object* m_object;
  Func          m_func;
};

template <typename Object, typename Result>
function_base<Result>*
mem_fn(Object* object, Result (Object::*func)()) {
  return new _mem_fn0<Object, Result>(object, func);
}

template <typename Object, typename Result>
function_base<Result>*
mem_fn(const Object* object, Result (Object::*func)() const) {
  return new _const_mem_fn0<Object, Result>(object, func);
}

}

#endif
