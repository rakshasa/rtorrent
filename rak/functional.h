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

#ifndef RAK_FUNCTIONAL_H
#define RAK_FUNCTIONAL_H

#include <functional>

namespace rak {

template <typename Type>
struct reference_fix {
  typedef Type type;
};

template <typename Type>
struct reference_fix<Type&> {
  typedef Type type;
};

template <typename Type>
struct value_t {
  value_t(Type v) : m_v(v) {}

  Type operator () () const { return m_v; }

  Type m_v;
};

template <typename Type>
inline value_t<Type>
value(Type v) {
  return value_t<Type>(v);
}

template <typename Type, typename Ftor>
struct accumulate_t {
  accumulate_t(Type t, Ftor f) : result(t), m_f(f) {}

  template <typename Arg>
  void operator () (const Arg& a) { result += m_f(a); }

  Type result;
  Ftor m_f;
};

template <typename Type, typename Ftor>
inline accumulate_t<Type, Ftor>
accumulate(Type t, Ftor f) {
  return accumulate_t<Type, Ftor>(t, f);
}

// Operators:

template <typename Type, typename Ftor>
struct equal_t {
  typedef bool result_type;

  equal_t(Type t, Ftor f) : m_t(t), m_f(f) {}

  template <typename Arg>
  bool operator () (Arg& a) {
    return m_t == m_f(a);
  }

  Type m_t;
  Ftor m_f;
};

template <typename Type, typename Ftor>
inline equal_t<Type, Ftor>
equal(Type t, Ftor f) {
  return equal_t<Type, Ftor>(t, f);
}

template <typename Type, typename Ftor>
struct not_equal_t {
  typedef bool result_type;

  not_equal_t(Type t, Ftor f) : m_t(t), m_f(f) {}

  template <typename Arg>
  bool operator () (Arg& a) {
    return m_t != m_f(a);
  }

  Type m_t;
  Ftor m_f;
};

template <typename Type, typename Ftor>
inline not_equal_t<Type, Ftor>
not_equal(Type t, Ftor f) {
  return not_equal_t<Type, Ftor>(t, f);
}

template <typename Type, typename Ftor>
struct less_t {
  typedef bool result_type;

  less_t(Type t, Ftor f) : m_t(t), m_f(f) {}

  template <typename Arg>
  bool operator () (Arg& a) {
    return m_t < m_f(a);
  }

  Type m_t;
  Ftor m_f;
};

template <typename Type, typename Ftor>
inline less_t<Type, Ftor>
less(Type t, Ftor f) {
  return less_t<Type, Ftor>(t, f);
}

template <typename Type, typename Ftor>
struct _greater {
  typedef bool result_type;

  _greater(Type t, Ftor f) : m_t(t), m_f(f) {}

  template <typename Arg>
  bool operator () (Arg& a) {
    return m_t > m_f(a);
  }

  Type m_t;
  Ftor m_f;
};

template <typename Type, typename Ftor>
inline _greater<Type, Ftor>
greater(Type t, Ftor f) {
  return _greater<Type, Ftor>(t, f);
}

template <typename Type, typename Ftor>
struct less_equal_t {
  typedef bool result_type;

  less_equal_t(Type t, Ftor f) : m_t(t), m_f(f) {}

  template <typename Arg>
  bool operator () (Arg& a) {
    return m_t <= m_f(a);
  }

  Type m_t;
  Ftor m_f;
};

template <typename Type, typename Ftor>
inline less_equal_t<Type, Ftor>
less_equal(Type t, Ftor f) {
  return less_equal_t<Type, Ftor>(t, f);
}

template <typename Type, typename Ftor>
struct greater_equal_t {
  typedef bool result_type;

  greater_equal_t(Type t, Ftor f) : m_t(t), m_f(f) {}

  template <typename Arg>
  bool operator () (Arg& a) {
    return m_t >= m_f(a);
  }

  Type m_t;
  Ftor m_f;
};

template <typename Type, typename Ftor>
inline greater_equal_t<Type, Ftor>
greater_equal(Type t, Ftor f) {
  return greater_equal_t<Type, Ftor>(t, f);
}

template <typename Src, typename Dest>
struct on_t : public std::unary_function<typename Src::argument_type, typename Dest::result_type> {
  typedef typename Dest::result_type result_type;

  on_t(Src s, Dest d) : m_dest(d), m_src(s) {}

  result_type operator () (typename reference_fix<typename Src::argument_type>::type arg) {
    return m_dest(m_src(arg));
  }

  Dest m_dest;
  Src m_src;
};
    
template <typename Src, typename Dest>
inline on_t<Src, Dest>
on(Src s, Dest d) {
  return on_t<Src, Dest>(s, d);
}  

// Creates a functor for accessing a member.
template <typename Class, typename Member>
struct mem_ptr_ref_t : public std::unary_function<Class&, Member&> {
  mem_ptr_ref_t(Member Class::*m) : m_member(m) {}

  Member& operator () (Class& c) {
    return c.*m_member;
  }

  const Member& operator () (const Class& c) {
    return c.*m_member;
  }

  Member Class::*m_member;
};

template <typename Class, typename Member>
inline mem_ptr_ref_t<Class, Member>
mem_ptr_ref(Member Class::*m) {
  return mem_ptr_ref_t<Class, Member>(m);
}

template <typename Cond, typename Then>
struct if_then_t {
  if_then_t(Cond c, Then t) : m_cond(c), m_then(t) {}

  template <typename Arg>
  void operator () (Arg& a) {
    if (m_cond(a))
      m_then(a);
  }

  Cond m_cond;
  Then m_then;
};

template <typename Cond, typename Then>
inline if_then_t<Cond, Then>
if_then(Cond c, Then t) {
  return if_then_t<Cond, Then>(c, t);
}

template <typename T>
struct call_delete : public std::unary_function<T*, void> {
  void operator () (T* t) {
    delete t;
  }
};

template <typename T>
inline void
call_delete_func(T* t) {
  delete t;
}

template <typename Operation>
class bind1st_t : public std::unary_function<typename Operation::second_argument_type,
					     typename Operation::result_type> {
protected:
  typedef typename reference_fix<typename Operation::first_argument_type>::type value_type;
  typedef typename reference_fix<typename Operation::second_argument_type>::type argument_type;

  Operation  m_op;
  value_type m_value;

public:
  bind1st_t(const Operation& op, const value_type v) :
    m_op(op), m_value(v) {}

  typename Operation::result_type
  operator () (const argument_type arg) {
    return m_op(m_value, arg);
  }
};	    

template <typename Operation, typename Type>
inline bind1st_t<Operation>
bind1st(const Operation& op, const Type& val) {
  return bind1st_t<Operation>(op, val);
}

template <typename Operation>
class bind2nd_t : public std::unary_function<typename Operation::first_argument_type,
					     typename Operation::result_type> {
protected:
  typedef typename reference_fix<typename Operation::first_argument_type>::type argument_type;
  typedef typename reference_fix<typename Operation::second_argument_type>::type value_type;

  Operation  m_op;
  value_type m_value;

public:
  bind2nd_t(const Operation& op, const value_type v) :
    m_op(op), m_value(v) {}

  typename Operation::result_type
  operator () (const argument_type arg) {
    return m_op(arg, m_value);
  }
};	    

template <typename Operation, typename Type>
inline bind2nd_t<Operation>
bind2nd(const Operation& op, const Type& val) {
  return bind2nd_t<Operation>(op, val);
}

// Lightweight callback function including pointer to object. Should
// be replaced by TR1 stuff later. Requires an object to bind, instead
// of using a seperate functor for that.

template <typename Object, typename Ret>
class mem_fn0 {
public:
  typedef Ret (Object::*Function)();

  mem_fn0() : m_object(NULL) {}
  mem_fn0(Object* o, Function f) : m_object(o), m_function(f) {}

  bool is_valid() const { return m_object; }

  Ret operator () () { return (m_object->*m_function)(); }
  
private:
  Object* m_object;
  Function m_function;
};

template <typename Object, typename Ret>
class const_mem_fn0 {
public:
  typedef Ret (Object::*Function)() const;

  const_mem_fn0() : m_object(NULL) {}
  const_mem_fn0(Object* o, Function f) : m_object(o), m_function(f) {}

  bool is_valid() const { return m_object; }

  Ret operator () () { return (m_object->*m_function)(); }
  
private:
  Object* m_object;
  Function m_function;
};

template <typename Object, typename Ret, typename Arg1>
class mem_fn1 {
public:
  typedef Ret (Object::*Function)(Arg1);

  mem_fn1() : m_object(NULL) {}
  mem_fn1(Object* o, Function f) : m_object(o), m_function(f) {}

  bool is_valid() const { return m_object; }

  Ret operator () (Arg1 a1) { return (m_object->*m_function)(a1); }
  
private:
  Object* m_object;
  Function m_function;
};

template <typename Object, typename Ret, typename Arg1, typename Arg2>
class mem_fn2 {
public:
  typedef Ret (Object::*Function)(Arg1, Arg2);

  mem_fn2() : m_object(NULL) {}
  mem_fn2(Object* o, Function f) : m_object(o), m_function(f) {}

  bool is_valid() const { return m_object; }

  Ret operator () (Arg1 a1, Arg2 a2) { return (m_object->*m_function)(a1, a2); }
  
private:
  Object* m_object;
  Function m_function;
};

template <typename Object, typename Ret, typename Arg1, typename Arg2, typename Arg3>
class mem_fn3 {
public:
  typedef Ret (Object::*Function)(Arg1, Arg2, Arg3);

  mem_fn3() : m_object(NULL) {}
  mem_fn3(Object* o, Function f) : m_object(o), m_function(f) {}

  bool is_valid() const { return m_object; }

  Ret operator () (Arg1 a1, Arg2 a2, Arg3 a3) { return (m_object->*m_function)(a1, a2, a3); }
  
private:
  Object* m_object;
  Function m_function;
};

template <typename Object, typename Ret>
inline mem_fn0<Object, Ret>
make_mem_fn(Object* o, Ret (Object::*f)()) {
 return mem_fn0<Object, Ret>(o, f);
}

template <typename Object, typename Ret>
inline const_mem_fn0<Object, Ret>
make_mem_fn(Object* o, Ret (Object::*f)() const) {
 return const_mem_fn0<Object, Ret>(o, f);
}

template <typename Object, typename Ret, typename Arg1>
inline mem_fn1<Object, Ret, Arg1>
make_mem_fn(Object* o, Ret (Object::*f)(Arg1)) {
 return mem_fn1<Object, Ret, Arg1>(o, f);
}

template <typename Object, typename Ret, typename Arg1, typename Arg2>
inline mem_fn2<Object, Ret, Arg1, Arg2>
make_mem_fn(Object* o, Ret (Object::*f)(Arg1, Arg2)) {
 return mem_fn2<Object, Ret, Arg1, Arg2>(o, f);
}

template <typename Object, typename Ret, typename Arg1, typename Arg2, typename Arg3>
inline mem_fn3<Object, Ret, Arg1, Arg2, Arg3>
make_mem_fn(Object* o, Ret (Object::*f)(Arg1, Arg2, Arg3)) {
 return mem_fn3<Object, Ret, Arg1, Arg2, Arg3>(o, f);
}

}

#endif
