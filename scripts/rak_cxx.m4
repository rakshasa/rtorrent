AC_DEFUN([RAK_CHECK_CXX11], [
  AC_ARG_ENABLE([c++0x],
    AC_HELP_STRING([--enable-c++0x], [compile with C++0x (unsupported)]),
    [
      if test "$enableval" = "yes"; then
        AX_CXX_COMPILE_STDCXX_0X
      else
        AX_CXX_COMPILE_STDCXX_11(noext)
      fi
    ],[
      AX_CXX_COMPILE_STDCXX_11(noext)
    ]
  )
])

AC_DEFUN([RAK_CHECK_TR1_LIB], [
  AC_LANG_PUSH(C++)
  AC_MSG_CHECKING(should use TR1 headers)

  AC_COMPILE_IFELSE([AC_LANG_SOURCE([
      #include <unordered_map>
      class Foo; typedef std::unordered_map<Foo*, int> Bar;
      Bar b1;
      ])
  ], [
      AC_MSG_RESULT(no)
      AC_DEFINE(USE_TR1_LIB, 0, Define to 1 if you need to use TR1 containers.)

      AC_DEFINE([lt_tr1_array], [<array>], [TR1 array])
      AC_DEFINE([lt_tr1_functional], [<functional>], [TR1 functional])
      AC_DEFINE([lt_tr1_memory], [<memory>], [TR1 memory])
      AC_DEFINE([lt_tr1_unordered_map], [<unordered_map>], [TR1 unordered_map])

  ], [
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([
        #include <tr1/unordered_map>
        class Foo; typedef std::tr1::unordered_map<Foo*, int> Bar;
        Bar b1;
        ])
    ], [
        AC_MSG_RESULT([yes])
        AC_DEFINE(USE_TR1_LIB, 1, Define to 1 if you need to use TR1 containers.)

        AC_DEFINE([lt_tr1_array], [<tr1/array>], [TR1 array])
        AC_DEFINE([lt_tr1_functional], [<tr1/functional>], [TR1 functional])
        AC_DEFINE([lt_tr1_memory], [<tr1/memory>], [TR1 memory])
        AC_DEFINE([lt_tr1_unordered_map], [<tr1/unordered_map>], [TR1 unordered_map])

    ], [
        AC_MSG_ERROR([No support for C++11 standard library nor TR1 extensions found.])
    ])
  ])

  AH_VERBATIM(lt_tr1_zzz, [
#if USE_TR1_LIB == 1
namespace std { namespace tr1 {} using namespace tr1; }
#endif
])

  AC_LANG_POP(C++)
])
