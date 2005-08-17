AC_DEFUN([TORRENT_CHECK_CXXFLAGS], [

  AC_MSG_CHECKING([for user-defined CXXFLAGS])

  if test -n "$CXXFLAGS"; then
    AC_MSG_RESULT([user-defined "$CXXFLAGS"])
  else
    CXXFLAGS="-O3 -Wall"
    AC_MSG_RESULT([default "$CXXFLAGS"])
  fi
])


AC_DEFUN([TORRENT_ENABLE_DEBUG], [
  AC_ARG_ENABLE(debug,
    [  --enable-debug          enable debug information [default=yes]],
    [
        if test "$enableval" = "yes"; then
            CXXFLAGS="$CXXFLAGS -g -DDEBUG"
        else
            CXXFLAGS="$CXXFLAGS -DNDEBUG"
        fi
    ],[
        CXXFLAGS="$CXXFLAGS -g -DDEBUG"
  ])
])


AC_DEFUN([TORRENT_ENABLE_WERROR], [
  AC_ARG_ENABLE(werror,
    [  --enable-werror         enable the -Werror flag [default=no]],
    [
        if test "$enableval" = "yes"; then
            CXXFLAGS="$CXXFLAGS -Werror"
        fi
  ])
])


AC_DEFUN([TORRENT_ENABLE_UNSAFE_OPTIMIZATION], [
  AC_ARG_ENABLE(unsafe-optimization,
    [  --enable-unsafe-optimization
                          enable unsafe optimization [default=yes]],
    [
        if test "$enableval" = "yes"; then
            AC_DEFINE(USE_UNSAFE_OPTIMIZATION, 1, Enable possibly unsafe optimization techniques.)
	else
            AC_DEFINE(USE_UNSAFE_OPTIMIZATION, 0, Disable possibly unsafe optimization techniques.)
        fi
    ],[
      AC_DEFINE(USE_UNSAFE_OPTIMIZATION, 0, Disable possibly unsafe optimization techniques.)
  ])
])


AC_DEFUN([TORRENT_OTFD], [
  AC_LANG_PUSH(C++)
  AC_MSG_CHECKING(for proper overloaded template function disambiguation)

  AC_COMPILE_IFELSE(
    [[template <typename T> void f(T&) {}
      template <typename T> void f(T*) {}
      int main() { int *i = 0; f(*i); f(i); }
    ]],
    [
      AC_MSG_RESULT(yes)
    ], [
      AC_MSG_RESULT(no)
      AC_MSG_ERROR([your compiler does not properly handle overloaded template function disambiguation])
  ])

  AC_LANG_POP(C++)
])


AC_DEFUN([TORRENT_MINCORE_SIGNEDNESS], [
  AC_LANG_PUSH(C++)
  AC_MSG_CHECKING(signedness of mincore parameter)

  AC_COMPILE_IFELSE(
    [[#include <sys/types.h>
      #include <sys/mman.h>
      void f() { mincore((void*)0, 0, (unsigned char*)0); }
    ]],
    [
      AC_DEFINE(USE_MINCORE_UNSIGNED, 1, use unsigned char* in mincore)
      AC_MSG_RESULT(unsigned)
    ],
    [
      AC_COMPILE_IFELSE(
        [[#include <sys/types.h>
          #include <sys/mman.h>
          void f() { mincore((void*)0, 0, (char*)0); }
        ]],
        [
          AC_DEFINE(USE_MINCORE_UNSIGNED, 0, use char* in mincore)
          AC_MSG_RESULT(signed)
        ],
        [
          AC_MSG_ERROR([mincore signedness test failed])
      ])
  ])

  AC_LANG_POP(C++)
])

AC_DEFUN([TORRENT_CHECK_EXECINFO], [
  AC_MSG_CHECKING(for execinfo.h)

  AC_COMPILE_IFELSE(
    [[#include <execinfo.h>
      int main() { backtrace((void**)0, 0); backtrace_symbols((char**)0, 0); return 0;}
    ]],
    [
      AC_MSG_RESULT(yes)
      AC_DEFINE(USE_EXECINFO, 1, Use execinfo.h)
    ], [
      AC_MSG_RESULT(no)
  ])
])

AC_DEFUN([TORRENT_CHECK_ALIGNED], [
  AC_MSG_CHECKING(the byte alignment)

  AC_RUN_IFELSE(
    [[#include <inttypes.h>
      int main() {
        char buf[5] = { 0, 0, 0, 0, 1 };
	int i;
        for (i = 0; i < 4; ++i)
	  if (*(uint32_t*)(buf + 1) == 0) return -1;
	return 0;
	}
    ]],
    [
      AC_MSG_RESULT(none needed)
    ], [
      AC_DEFINE(USE_ALIGNED, 1, Require byte alignment)
      AC_MSG_RESULT(required)
  ])
])

