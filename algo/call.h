// -*- c++ -*-
/* Do not edit! -- generated file */

#ifndef ALGO_CALL_H
#define ALGO_CALL_H

#include <algo/bits/common.h>
#include <algo/bits/impl_call.h>

namespace algo {

template <typename FuncRet>
inline _wrapper<
  _ptr_fun0<FuncRet (*)(), FuncRet > >

ptr_fun(FuncRet (*func)()) {

  return make_wrapper(_ptr_fun0<
		      FuncRet (*)(), FuncRet >
		      (func));
}

template <typename FuncParm1, typename FuncRet, typename Parm1>
inline _wrapper<
  _ptr_fun1<FuncRet (*)(FuncParm1), FuncRet, Parm1 > >

ptr_fun(FuncRet (*func)(FuncParm1), _wrapper<Parm1> parm1) {

  return make_wrapper(_ptr_fun1<
		      FuncRet (*)(FuncParm1), FuncRet, Parm1 >
		      (func, parm1.m_ftor));
}

template <typename FuncParm1,typename FuncParm2, typename FuncRet, typename Parm1,typename Parm2>
inline _wrapper<
  _ptr_fun2<FuncRet (*)(FuncParm1,FuncParm2), FuncRet, Parm1,Parm2 > >

ptr_fun(FuncRet (*func)(FuncParm1,FuncParm2), _wrapper<Parm1> parm1,_wrapper<Parm2> parm2) {

  return make_wrapper(_ptr_fun2<
		      FuncRet (*)(FuncParm1,FuncParm2), FuncRet, Parm1,Parm2 >
		      (func, parm1.m_ftor,parm2.m_ftor));
}

template <typename FuncParm1,typename FuncParm2,typename FuncParm3, typename FuncRet, typename Parm1,typename Parm2,typename Parm3>
inline _wrapper<
  _ptr_fun3<FuncRet (*)(FuncParm1,FuncParm2,FuncParm3), FuncRet, Parm1,Parm2,Parm3 > >

ptr_fun(FuncRet (*func)(FuncParm1,FuncParm2,FuncParm3), _wrapper<Parm1> parm1,_wrapper<Parm2> parm2,_wrapper<Parm3> parm3) {

  return make_wrapper(_ptr_fun3<
		      FuncRet (*)(FuncParm1,FuncParm2,FuncParm3), FuncRet, Parm1,Parm2,Parm3 >
		      (func, parm1.m_ftor,parm2.m_ftor,parm3.m_ftor));
}

template <typename FuncParm1,typename FuncParm2,typename FuncParm3,typename FuncParm4, typename FuncRet, typename Parm1,typename Parm2,typename Parm3,typename Parm4>
inline _wrapper<
  _ptr_fun4<FuncRet (*)(FuncParm1,FuncParm2,FuncParm3,FuncParm4), FuncRet, Parm1,Parm2,Parm3,Parm4 > >

ptr_fun(FuncRet (*func)(FuncParm1,FuncParm2,FuncParm3,FuncParm4), _wrapper<Parm1> parm1,_wrapper<Parm2> parm2,_wrapper<Parm3> parm3,_wrapper<Parm4> parm4) {

  return make_wrapper(_ptr_fun4<
		      FuncRet (*)(FuncParm1,FuncParm2,FuncParm3,FuncParm4), FuncRet, Parm1,Parm2,Parm3,Parm4 >
		      (func, parm1.m_ftor,parm2.m_ftor,parm3.m_ftor,parm4.m_ftor));
}


template <typename Target, typename FuncRet, typename FuncClass>
inline _wrapper<
  _mem_fun0<Target, FuncRet (FuncClass::*)(), FuncRet > >

mem_fun(_wrapper<Target> target, FuncRet (FuncClass::*func)()) {

  return make_wrapper(_mem_fun0<
		      Target, FuncRet (FuncClass::*)(), FuncRet >
		      (target.m_ftor, func));
}

template <typename FuncParm1, typename Target, typename FuncRet, typename FuncClass, typename Parm1>
inline _wrapper<
  _mem_fun1<Target, FuncRet (FuncClass::*)(FuncParm1), FuncRet, Parm1 > >

mem_fun(_wrapper<Target> target, FuncRet (FuncClass::*func)(FuncParm1), _wrapper<Parm1> parm1) {

  return make_wrapper(_mem_fun1<
		      Target, FuncRet (FuncClass::*)(FuncParm1), FuncRet, Parm1 >
		      (target.m_ftor, func, parm1.m_ftor));
}

template <typename FuncParm1,typename FuncParm2, typename Target, typename FuncRet, typename FuncClass, typename Parm1,typename Parm2>
inline _wrapper<
  _mem_fun2<Target, FuncRet (FuncClass::*)(FuncParm1,FuncParm2), FuncRet, Parm1,Parm2 > >

mem_fun(_wrapper<Target> target, FuncRet (FuncClass::*func)(FuncParm1,FuncParm2), _wrapper<Parm1> parm1,_wrapper<Parm2> parm2) {

  return make_wrapper(_mem_fun2<
		      Target, FuncRet (FuncClass::*)(FuncParm1,FuncParm2), FuncRet, Parm1,Parm2 >
		      (target.m_ftor, func, parm1.m_ftor,parm2.m_ftor));
}

template <typename FuncParm1,typename FuncParm2,typename FuncParm3, typename Target, typename FuncRet, typename FuncClass, typename Parm1,typename Parm2,typename Parm3>
inline _wrapper<
  _mem_fun3<Target, FuncRet (FuncClass::*)(FuncParm1,FuncParm2,FuncParm3), FuncRet, Parm1,Parm2,Parm3 > >

mem_fun(_wrapper<Target> target, FuncRet (FuncClass::*func)(FuncParm1,FuncParm2,FuncParm3), _wrapper<Parm1> parm1,_wrapper<Parm2> parm2,_wrapper<Parm3> parm3) {

  return make_wrapper(_mem_fun3<
		      Target, FuncRet (FuncClass::*)(FuncParm1,FuncParm2,FuncParm3), FuncRet, Parm1,Parm2,Parm3 >
		      (target.m_ftor, func, parm1.m_ftor,parm2.m_ftor,parm3.m_ftor));
}

template <typename FuncParm1,typename FuncParm2,typename FuncParm3,typename FuncParm4, typename Target, typename FuncRet, typename FuncClass, typename Parm1,typename Parm2,typename Parm3,typename Parm4>
inline _wrapper<
  _mem_fun4<Target, FuncRet (FuncClass::*)(FuncParm1,FuncParm2,FuncParm3,FuncParm4), FuncRet, Parm1,Parm2,Parm3,Parm4 > >

mem_fun(_wrapper<Target> target, FuncRet (FuncClass::*func)(FuncParm1,FuncParm2,FuncParm3,FuncParm4), _wrapper<Parm1> parm1,_wrapper<Parm2> parm2,_wrapper<Parm3> parm3,_wrapper<Parm4> parm4) {

  return make_wrapper(_mem_fun4<
		      Target, FuncRet (FuncClass::*)(FuncParm1,FuncParm2,FuncParm3,FuncParm4), FuncRet, Parm1,Parm2,Parm3,Parm4 >
		      (target.m_ftor, func, parm1.m_ftor,parm2.m_ftor,parm3.m_ftor,parm4.m_ftor));
}



}

#endif
