AC_DEFUN([TORRENT_CHECK_CXXFLAGS], [

  AC_MSG_CHECKING([for user-defined CXXFLAGS])

  if test -n "$CXXFLAGS"; then
    AC_MSG_RESULT([user-defined "$CXXFLAGS"])
  else
    CXXFLAGS="-O2 -Wall"
    AC_MSG_RESULT([default "$CXXFLAGS"])
  fi
])


AC_DEFUN([TORRENT_ENABLE_DEBUG], [
  AC_ARG_ENABLE(debug,
    [  --enable-debug          enable debug information [[default=yes]]],
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
    [  --enable-werror         enable the -Werror and -Wall flag [[default=no]]],
    [
        if test "$enableval" = "yes"; then
            CXXFLAGS="$CXXFLAGS -Werror -Wall"
        fi
  ])
])


AC_DEFUN([TORRENT_ENABLE_EXTRA_DEBUG], [
  AC_ARG_ENABLE(extra-debug,
    [  --enable-extra-debug    enable extra debugging checks. [[default=no]]],
    [
        if test "$enableval" = "yes"; then
            AC_DEFINE(USE_EXTRA_DEBUG, 1, Enable extra debugging checks.)
        fi
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
      AC_DEFINE(USE_MINCORE, 1, Use mincore)
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
          AC_DEFINE(USE_MINCORE, 1, Use mincore)
          AC_DEFINE(USE_MINCORE_UNSIGNED, 0, use char* in mincore)
          AC_MSG_RESULT(signed)
        ],
        [
          AC_MSG_ERROR([failed, do *not* attempt to use --disable-mincore unless you are running Win32.])
      ])
  ])

  AC_LANG_POP(C++)
])

AC_DEFUN([TORRENT_MINCORE], [
  AC_ARG_ENABLE(mincore,
    [  --disable-mincore       disable mincore check [[default=enable]]],
    [
      if test "$enableval" = "yes"; then
        TORRENT_MINCORE_SIGNEDNESS()
      else
	AC_MSG_CHECKING(for mincore)
	AC_MSG_RESULT(disabled)
      fi
    ],[
        TORRENT_MINCORE_SIGNEDNESS()
    ])
])

AC_DEFUN([TORRENT_CHECK_MADVISE], [
  AC_MSG_CHECKING(for madvise)

  AC_COMPILE_IFELSE(
    [[#include <sys/types.h>
          #include <sys/mman.h>
          void f() { static char test[1024]; madvise((void *)test, sizeof(test), MADV_NORMAL); }
    ]],
    [
      AC_MSG_RESULT(yes)
      AC_DEFINE(USE_MADVISE, 1, Use madvise)
    ], [
      AC_MSG_RESULT(no)
  ])
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
        char buf[8] = { 0, 0, 0, 0, 1, 0, 0, 0 };
	int i;
        for (i = 1; i < 4; ++i)
	  if (*(uint32_t*)(buf + i) == 0) return -1;
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

AC_DEFUN([TORRENT_ENABLE_ALIGNED], [
  AC_ARG_ENABLE(aligned,
    [  --enable-aligned        enable alignment safe code [[default=check]]],
    [
        if test "$enableval" = "yes"; then
          AC_DEFINE(USE_ALIGNED, 1, Require byte alignment)
        fi
    ],[
        TORRENT_CHECK_ALIGNED
  ])
])
