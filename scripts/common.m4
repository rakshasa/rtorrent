AC_DEFUN([TORRENT_WITH_SYSROOT], [
  AC_ARG_WITH(sysroot,
    AS_HELP_STRING([--with-sysroot=PATH],
      [compile and link with a specific sysroot]),
    [
      AC_MSG_CHECKING(for sysroot)

      if test "$withval" = "no"; then
        AC_MSG_RESULT(no)

      elif test "$withval" = "yes"; then
        AC_MSG_RESULT(not a path)
        AC_MSG_ERROR(The sysroot option must point to a directory, like f.ex "/Developer/SDKs/MacOSX10.4u.sdk".)
      else
        AC_MSG_RESULT($withval)
        
        CXXFLAGS="$CXXFLAGS -isysroot $withval"
        LDFLAGS="$LDFLAGS -Wl,-syslibroot,$withval"
      fi
    ])
])


AC_DEFUN([TORRENT_ENABLE_ARCH], [
  AC_ARG_ENABLE(arch,
    AS_HELP_STRING([--enable-arch=ARCH],
      [comma seprated list of architectures to compile for]),
    [
      AC_MSG_CHECKING(for target architectures)

      if test "$enableval" = "yes"; then
        AC_MSG_ERROR(no arch supplied)

      elif test "$enableval" = "no"; then
        AC_MSG_RESULT(using default)

      else
        AC_MSG_RESULT($enableval)

        for i in `IFS=,; echo $enableval`; do
          CFLAGS="$CFLAGS -march=$i"
          CXXFLAGS="$CXXFLAGS -march=$i"
          LDFLAGS="$LDFLAGS -march=$i"
        done
      fi
    ])
])


AC_DEFUN([TORRENT_MINCORE_SIGNEDNESS], [
  AC_LANG_PUSH(C++)
  AC_MSG_CHECKING(signedness of mincore parameter)

  AC_COMPILE_IFELSE([AC_LANG_SOURCE([
      #include <sys/types.h>
      #include <sys/mman.h>
      #include <unistd.h>
      void f() { mincore((char*)0, 0, (unsigned char*)0); }
      ])],
    [
      AC_DEFINE(USE_MINCORE, 1, Use mincore)
      AC_DEFINE(USE_MINCORE_UNSIGNED, 1, use unsigned char* in mincore)
      AC_MSG_RESULT(unsigned)
    ],
    [
      AC_COMPILE_IFELSE([AC_LANG_SOURCE([
          #include <sys/types.h>
          #include <sys/mman.h>
          #include <unistd.h>
          void f() { mincore((char*)0, 0, (char*)0); }
          ])],
        [
          AC_DEFINE(USE_MINCORE, 1, Use mincore)
          AC_DEFINE(USE_MINCORE_UNSIGNED, 0, use char* in mincore)
          AC_MSG_RESULT(signed)
        ],
        [
          AC_MSG_ERROR([failed, do *not* attempt fix this with --disable-mincore unless you are running Win32.])
      ])
  ])

  AC_LANG_POP(C++)
])

AC_DEFUN([TORRENT_MINCORE], [
  AC_ARG_ENABLE(mincore,
    AS_HELP_STRING([--disable-mincore],
      [disable mincore check [[default=enable]]]),
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

  AC_COMPILE_IFELSE([AC_LANG_SOURCE([
      #include <sys/types.h>
      #include <sys/mman.h>
        void f() { static char test@<:@1024@:>@; madvise((void *)test, sizeof(test), MADV_NORMAL); }
      ])],
    [
      AC_MSG_RESULT(yes)
      AC_DEFINE(USE_MADVISE, 1, Use madvise)
    ], [
      AC_MSG_RESULT(no)
  ])
])

AC_DEFUN([TORRENT_CHECK_POPCOUNT], [
  AC_MSG_CHECKING(for __builtin_popcount)

  AC_COMPILE_IFELSE([AC_LANG_SOURCE([
      int f() { return __builtin_popcount(0); }
    ])],
    [
      AC_MSG_RESULT(yes)
      AC_DEFINE(USE_BUILTIN_POPCOUNT, 1, Use __builtin_popcount.)
    ], [
      AC_MSG_RESULT(no)
  ])
])

AC_DEFUN([TORRENT_CHECK_CACHELINE], [
  AC_MSG_CHECKING(for cacheline)

  AC_COMPILE_IFELSE([AC_LANG_SOURCE([
      #include <stdlib.h>
          #include <linux/cache.h>
          void* vptr __cacheline_aligned;
          void f() { posix_memalign(&vptr, SMP_CACHE_BYTES, 42); }
      ])],
    [
      AC_MSG_RESULT(found builtin)
dnl      AC_DEFINE(LT_SMP_CACHE_BYTES, SMP_CACHE_BYTES, Largest L1 cache size we know of, should work on all archs.)
dnl      AC_DEFINE(lt_cacheline_aligned, __cacheline_aligned, LibTorrent defined cacheline aligned.)

dnl   Need to fix this so that it uses the stuff defined by the system.

      AC_DEFINE(LT_SMP_CACHE_BYTES, 128, Largest L1 cache size we know of should work on all archs.)
      AC_DEFINE(lt_cacheline_aligned, __attribute__((__aligned__(LT_SMP_CACHE_BYTES))), LibTorrent defined cacheline aligned.)
    ], [
      AC_MSG_RESULT(using default 128 bytes)
      AC_DEFINE(LT_SMP_CACHE_BYTES, 128, Largest L1 cache size we know of should work on all archs.)
      AC_DEFINE(lt_cacheline_aligned, __attribute__((__aligned__(LT_SMP_CACHE_BYTES))), LibTorrent defined cacheline aligned.)
  ])
])

AC_DEFUN([TORRENT_CHECK_ALIGNED], [
  AC_MSG_CHECKING(the byte alignment)

  AC_RUN_IFELSE([AC_LANG_SOURCE([
      #include <inttypes.h>
      int main() {
        char buf@<:@8@:>@ = { 0, 0, 0, 0, 1, 0, 0, 0 };
	int i;
        for (i = 1; i < 4; ++i)
	  if (*(uint32_t*)(buf + i) == 0) return -1;
	return 0;
	}
      ])],
    [
      AC_MSG_RESULT(none needed)
    ], [
      AC_DEFINE(USE_ALIGNED, 1, Require byte alignment)
      AC_MSG_RESULT(required)
  ])
])


AC_DEFUN([TORRENT_ENABLE_ALIGNED], [
  AC_ARG_ENABLE(aligned,
    AS_HELP_STRING([--enable-aligned],
      [enable alignment safe code [[default=check]]]),
    [
        if test "$enableval" = "yes"; then
          AC_DEFINE(USE_ALIGNED, 1, Require byte alignment)
        fi
    ],[
        TORRENT_CHECK_ALIGNED
  ])
])


AC_DEFUN([TORRENT_DISABLE_INSTRUMENTATION], [
  AC_MSG_CHECKING([if instrumentation should be included])

  AC_ARG_ENABLE(instrumentation,
    AS_HELP_STRING([--disable-instrumentation],
      [disable instrumentation [[default=enabled]]]),
    [
      if test "$enableval" = "yes"; then
        AC_DEFINE(LT_INSTRUMENTATION, 1, enable instrumentation)
	AC_MSG_RESULT(yes)
      else
	AC_MSG_RESULT(no)
      fi
    ],[
      AC_DEFINE(LT_INSTRUMENTATION, 1, enable instrumentation)
      AC_MSG_RESULT(yes)
    ])
])


AC_DEFUN([TORRENT_ENABLE_INTERRUPT_SOCKET], [
  AC_ARG_ENABLE(interrupt-socket,
    AS_HELP_STRING([--enable-interrupt-socket],
      [enable interrupt socket [[default=no]]]),
    [
      if test "$enableval" = "yes"; then
        AC_DEFINE(USE_INTERRUPT_SOCKET, 1, Use interrupt socket instead of pthread_kill)
      fi
    ]
  )
])

AC_DEFUN([TORRENT_DISABLE_IPV6], [
  AC_ARG_ENABLE(ipv6,
    AS_HELP_STRING([--enable-ipv6],
      [enable ipv6 [[default=no]]]),
    [
        if test "$enableval" = "yes"; then
            AC_DEFINE(RAK_USE_INET6, 1, enable ipv6 stuff)
        fi
    ])
])
