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

namespace rak {

template <typename _Result>
class function_base {
public:
  virtual ~function_base() {}

  virtual _Result operator () () = 0;
};

template <typename _Base>
struct function_ref {
  explicit function_ref(_Base* b) : m_base(b) {}

  _Base* m_base;
};

template <typename _Result>
class function {
public:
  typedef _Result result_type;
  typedef function_base<_Result> _Base;

  function() : m_base(0) {}
  function(function_ref<_Base> f) : m_base(f.m_base) {}
  explicit function(function_base<_Result>* base) { m_base = base; }
  
  ~function()                                     { delete m_base; }

  function& operator = (function& f) { m_base = f.release(); return *this; }

  function& operator = (function_ref<_Base> f) {
    if (m_base != f.m_base) {
      delete m_base;
      m_base = f.m_base;
    }

    return *this;
  }

  template <typename _T>
  operator function_ref<_T> () { return function_ref<_T>(this->release()); }

  _Result operator () () { return (*m_base)(); }

private:
  _Base* release() { _Base* tmp = m_base; m_base = 0; return tmp; }

  _Base* m_base;
};

template <typename _Object, typename _Result>
class _mem_fn0 : public function_base<_Result> {
public:
  typedef _Result (_Object::*_Func)();

  _mem_fn0(_Object* object, _Func func) : m_object(object), m_func(func) {}
  virtual ~_mem_fn0() {}
  
  virtual _Result operator () () { return (m_object->*m_func)(); }

private:
  _Object* m_object;
  _Func    m_func;
};

template <typename _Object, typename _Result>
class _const_mem_fn0 : public function_base<_Result> {
public:
  typedef _Result (_Object::*_Func)() const;

  _const_mem_fn0(const _Object* object, _Func func) : m_object(object), m_func(func) {}
  virtual ~_const_mem_fn0() {}
  
  virtual _Result operator () () { return (m_object->*m_func)(); }

private:
  const _Object* m_object;
  _Func          m_func;
};

template <typename _Object, typename _Result>
function<_Result>
mem_fn(_Object* object, _Result (_Object::*func)()) {
  return function<_Result>(static_cast<function_base<_Result>*>(new _mem_fn0<_Object, _Result>(object, func)));
}

template <typename _Object, typename _Result>
function<_Result>
mem_fn(const _Object* object, _Result (_Object::*func)() const) {
  return function<_Result>(static_cast<function_base<_Result>*>(new _const_mem_fn0<_Object, _Result>(object, func)));
}

}

#endif
