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


AC_DEFUN([TORRENT_REMOVE_UNWANTED],
[
  AC_REQUIRE([AC_PROG_GREP])
  values_to_check=`for i in $2; do echo $i; done`
  unwanted_values=`for i in $3; do echo $i; done`
  if test -z "${unwanted_values}"; then
    $1="$2"
  else
    result=`echo "${values_to_check}" | $GREP -Fvx -- "${unwanted_values}" | $GREP -v '^$'`
    # join with spaces, squeeze repeats, and trim trailing space
    $1=$(printf '%s\n' "$result" | tr '\n' ' ' | sed 's/  */ /g; s/ *$//')
  fi
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
          AC_MSG_ERROR([failed, do *not* attempt fix this with --disable-mincore unless you are running Win32 or OpenBSD.])
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

AC_DEFUN([TORRENT_CHECK_POSIX_FADVISE], [
  AC_MSG_CHECKING(for posix_fadvise)

  AC_COMPILE_IFELSE([AC_LANG_SOURCE([
      #include <fcntl.h>
      void f() { posix_fadvise(0, 0, 0, POSIX_FADV_RANDOM); }
      ])],
    [
      AC_MSG_RESULT(yes)
      AC_DEFINE(USE_POSIX_FADVISE, 1, Use posix_fadvise)
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
  AC_REQUIRE([AC_CANONICAL_HOST])
  AC_MSG_CHECKING([for target cacheline size])

  case "$host_os" in
    linux*)
      # REGION: Linux Kernel Extraction Loop
      AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
        #include <stdlib.h>
        #include <linux/cache.h>
        void* vptr;
        void f() {
          int res = posix_memalign(&vptr, SMP_CACHE_BYTES, 42);
          (void)res;
        }
      ]])],[
        # We need an explicit variable fallback condition inside AC_COMPUTE_INT
        AC_COMPUTE_INT([torrent_cv_cacheline_size], [SMP_CACHE_BYTES], [#include <linux/cache.h>], [torrent_cv_cacheline_size=0])

        if test "$torrent_cv_cacheline_size" -gt 0; then
          AC_MSG_RESULT([linux builtin ($torrent_cv_cacheline_size bytes)])
          AC_DEFINE_UNQUOTED([LT_SMP_CACHE_BYTES], [$torrent_cv_cacheline_size], [System-defined Linux L1 SMP cacheline size.])
        else
          # Handle scenarios where macro maps to a complex runtime expression or fails
          AC_MSG_RESULT([failed to parse SMP_CACHE_BYTES value])
          AC_MSG_FAILURE([Linux kernel headers found, but cacheline constant could not be computed at compile-time.])
        fi
      ],[
        # Explicitly validate the CPU type even on Linux if the header check fails
        case "$host_cpu" in
          x86_64*|amd64*|i386*|i486*|i586*|i686*)
            AC_MSG_RESULT([linux fallback x86 64 bytes])
            AC_DEFINE([LT_SMP_CACHE_BYTES], 64, [Fallback 64-byte alignment for Linux x86 hardware.])
            ;;
          arm*|aarch64*|powerpc*|ppc*|s390x*)
            AC_MSG_RESULT([linux fallback enterprise 128 bytes])
            AC_DEFINE([LT_SMP_CACHE_BYTES], 128, [Fallback 128-byte alignment for Linux enterprise hardware.])
            ;;
          *)
            AC_MSG_RESULT([unrecognized CPU arch on Linux header fallback])
            AC_MSG_FAILURE([Unrecognized CPU architecture ($host_cpu) on Linux fallback path. Aborting build.])
            ;;
        esac
      ])
      ;;

    *)
      # REGION: Cross-Platform Strict Hardware Mapping (macOS, FreeBSD, OpenBSD, NetBSD)
      case "$host_cpu" in
        x86_64*|amd64*|i386*|i486*|i586*|i686*)
          # Explicit x86 desktop hardware baseline block
          AC_MSG_RESULT([$host_os ($host_cpu) standard x86 64 bytes])
          AC_DEFINE([LT_SMP_CACHE_BYTES], 64, [Standard 64-byte alignment for stable x86 hardware layout.])
          ;;
        arm*|aarch64*|powerpc*|ppc*|s390x*)
          # Explicit modern enterprise and Apple Silicon hardware baseline block
          AC_MSG_RESULT([$host_os ($host_cpu) stable enterprise 128 bytes])
          AC_DEFINE([LT_SMP_CACHE_BYTES], 128, [Optimized 128-byte alignment for newer high-performance chipsets.])
          ;;
        *)
          # STRICT ENFORCEMENT: Fail the build immediately if the CPU isn't explicitly known
          AC_MSG_RESULT([unrecognized architecture])
          AC_MSG_FAILURE([The target CPU architecture ($host_cpu) is unrecognized. Aborting configuration to prevent fatal runtime false-sharing or memory misalignment errors.])
          ;;
      esac
      ;;
  esac
])


AC_DEFUN([TORRENT_CHECK_CUSTOM_ENDIAN64], [
  AC_CHECK_HEADERS([endian.h sys/endian.h libkern/OSByteOrder.h])
  AC_C_BIGENDIAN

  AH_TEMPLATE([CUSTOM_ENDIAN_HEADER], [The system header to include for endian conversions.])
  AH_TEMPLATE([custom_ntohll], [Convert 64-bit integer from network to host byte order.])
  AH_TEMPLATE([custom_htonll], [Convert 64-bit integer from host to network byte order.])

  dnl 1. Initialize our header variable tracking
  endian_header_file='<stdint.h>'

  dnl 2. Test if native be64toh works natively via <endian.h> (Standard Linux/glibc)
  AC_MSG_CHECKING([for native be64toh via endian.h])
  AC_LINK_IFELSE([AC_LANG_SOURCE([
      #include <stdint.h>
      #ifdef HAVE_ENDIAN_H
      #  include <endian.h>
      #endif
      int main() { uint64_t x = be64toh(1); return 0; }
    ])],
    [has_native_be64=yes; endian_header_file='<endian.h>'],
    [has_native_be64=no]
  )
  AC_MSG_RESULT([$has_native_be64])

  dnl 3. If missing, test if it works via <sys/endian.h> (True BSD systems like FreeBSD)
  if test "$has_native_be64" = "no"; then
    AC_MSG_CHECKING([for native be64toh via sys/endian.h])
    AC_LINK_IFELSE([AC_LANG_SOURCE([
        #include <stdint.h>
        #ifdef HAVE_SYS_ENDIAN_H
        #  include <sys/endian.h>
        #endif
        int main() { uint64_t x = be64toh(1); return 0; }
      ])],
      [has_native_be64=yes; endian_header_file='<sys/endian.h>'],
      [has_native_be64=no]
    )
    AC_MSG_RESULT([$has_native_be64])
  fi

  dnl 4. Route definitions cleanly based on actual testing results
  if test "$has_native_be64" = "yes"; then
    dnl Linux or true BSD environment
    AC_DEFINE_UNQUOTED([CUSTOM_ENDIAN_HEADER], [$endian_header_file], [The header containing native endian macros.])
    AC_DEFINE([custom_ntohll(x)], [(be64toh(x))], [Use native platform be64toh])
    AC_DEFINE([custom_htonll(x)], [(htobe64(x))], [Use native platform htobe64])
  else
    dnl Fallback block: Check if we are on macOS using Apple's optimized hardware libraries
    if test "$ac_cv_header_libkern_OSByteOrder_h" = "yes"; then
      AC_DEFINE([CUSTOM_ENDIAN_HEADER], [<libkern/OSByteOrder.h>], [The header containing native endian macros.])
      AC_DEFINE([custom_ntohll(x)], [(OSSwapBigToHostInt64(x))], [Map missing be64toh to macOS native swap])
      AC_DEFINE([custom_htonll(x)], [(OSSwapHostToBigInt64(x))], [Map missing htobe64 to macOS native swap])
    else
      dnl Fallback for platforms with no built-in architecture macros (compiler built-ins)
      AC_DEFINE([CUSTOM_ENDIAN_HEADER], [<stdint.h>], [The header containing native endian macros.])
      if test "$ac_cv_c_bigendian" = "yes"; then
        AC_DEFINE([custom_ntohll(x)], [((uint64_t)(x))], [Fallback 64-bit conversion.])
        AC_DEFINE([custom_htonll(x)], [((uint64_t)(x))], [Fallback 64-bit conversion.])
      else
        AC_DEFINE([custom_ntohll(x)], [(__builtin_bswap64((uint64_t)(x)))], [Fallback 64-bit conversion.])
        AC_DEFINE([custom_htonll(x)], [(__builtin_bswap64((uint64_t)(x)))], [Fallback 64-bit conversion.])
      fi
    fi
  fi
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


AC_DEFUN([TORRENT_ENABLE_CUSTOM_STACK_SIZE], [
  AC_ARG_ENABLE([pthread-setstacksize],
    [AS_HELP_STRING([--enable-pthread-setstacksize], [explicitly set pthread stack size (auto-enabled for musl)])],
    [enable_pthread_setstacksize=$enableval],
    [enable_pthread_setstacksize=auto])

  AC_CANONICAL_HOST
  if test "x$enable_pthread_setstacksize" = "xauto"; then
    case "$host" in
      *-musl*)
        enable_pthread_setstacksize=yes
        ;;
      *)
        enable_pthread_setstacksize=no
        ;;
    esac
  fi

  if test "x$enable_pthread_setstacksize" = "xyes"; then
    AC_DEFINE([USE_PTHREAD_SETSTACKSIZE], [1], [Use explicit pthread stack size])
    AC_DEFINE([DEFAULT_PTHREAD_STACKSIZE], [(8 * 1024 * 1024)], [Default pthread stack size in bytes])
  fi
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
