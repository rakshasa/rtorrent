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
