// -*- c++ -*-
/* Do not edit! -- generated file */

#ifndef ALGO_CALL_H
#define ALGO_CALL_H

#include <algo/bits/common.h>

namespace algo {

template <typename Func, typename T1 = _nil,typename T2 = _nil,typename T3 = _nil,typename T4 = _nil>
struct test_args;

struct test_args0 {
  static const int result = 1;
};

template <typename T1, typename P1>
struct test_args1 {

  static _one test(P1);
  static _two test(...);

  static const int result = sizeof(test(_makeT<T1>()))
    == 1 ? 1 : 0;
};

template <typename T1,typename T2, typename P1,typename P2>
struct test_args2 {

  static _one test(P1,P2);
  static _two test(...);

  static const int result = sizeof(test(_makeT<T1>(),_makeT<T2>()))
    == 1 ? 1 : 0;
};

template <typename T1,typename T2,typename T3, typename P1,typename P2,typename P3>
struct test_args3 {

  static _one test(P1,P2,P3);
  static _two test(...);

  static const int result = sizeof(test(_makeT<T1>(),_makeT<T2>(),_makeT<T3>()))
    == 1 ? 1 : 0;
};

template <typename T1,typename T2,typename T3,typename T4, typename P1,typename P2,typename P3,typename P4>
struct test_args4 {

  static _one test(P1,P2,P3,P4);
  static _two test(...);

  static const int result = sizeof(test(_makeT<T1>(),_makeT<T2>(),_makeT<T3>(),_makeT<T4>()))
    == 1 ? 1 : 0;
};



template <typename Ret>
struct test_args<Ret (*)()> {

  static const int result = 1;
};

template <typename P1, typename T1,
	  typename Ret>
struct test_args<Ret (*)(P1), T1> {

  static const int result = test_args1<T1, P1>::result;
};

template <typename P1,typename P2, typename T1,typename T2,
	  typename Ret>
struct test_args<Ret (*)(P1,P2), T1,T2> {

  static const int result = test_args2<T1,T2, P1,P2>::result;
};

template <typename P1,typename P2,typename P3, typename T1,typename T2,typename T3,
	  typename Ret>
struct test_args<Ret (*)(P1,P2,P3), T1,T2,T3> {

  static const int result = test_args3<T1,T2,T3, P1,P2,P3>::result;
};

template <typename P1,typename P2,typename P3,typename P4, typename T1,typename T2,typename T3,typename T4,
	  typename Ret>
struct test_args<Ret (*)(P1,P2,P3,P4), T1,T2,T3,T4> {

  static const int result = test_args4<T1,T2,T3,T4, P1,P2,P3,P4>::result;
};


}

#endif
