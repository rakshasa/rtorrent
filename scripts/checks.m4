AC_DEFUN([TORRENT_CHECK_XFS], [
  AC_MSG_CHECKING(for XFS support)

  AC_COMPILE_IFELSE([AC_LANG_SOURCE([
      #include <xfs/libxfs.h>
      #include <sys/ioctl.h>
      int main() {
        struct xfs_flock64 l;
        ioctl(0, XFS_IOC_RESVSP64, &l);
        return 0;
      }
      ])],
    [
      AC_DEFINE(USE_XFS, 1, Use XFS filesystem stuff.)
      AC_MSG_RESULT(yes)
    ], [
      AC_MSG_RESULT(no)
    ])
])


AC_DEFUN([TORRENT_WITHOUT_XFS], [
  AC_ARG_WITH(xfs,
    AS_HELP_STRING([--without-xfs],[do not check for XFS filesystem support]),
    [
       if test "$withval" = "yes"; then
        TORRENT_CHECK_XFS
      fi
    ], [
        TORRENT_CHECK_XFS
    ])
])


AC_DEFUN([TORRENT_WITH_XFS], [
  AC_ARG_WITH(xfs,
    AS_HELP_STRING([--with-xfs],[check for XFS filesystem support]),
    [
      if test "$withval" = "yes"; then
        TORRENT_CHECK_XFS
      fi
    ])
])


AC_DEFUN([TORRENT_CHECK_EPOLL], [
  AC_MSG_CHECKING(for epoll support)

  AC_COMPILE_IFELSE([AC_LANG_SOURCE([
      #include <sys/epoll.h>
      int main() {
        int fd = epoll_create(100);
        return 0;
      }
      ])],
    [
      AC_DEFINE(USE_EPOLL, 1, Use epoll.)
      AC_MSG_RESULT(yes)
    ], [
      AC_MSG_RESULT(no)
    ])
])

AC_DEFUN([TORRENT_WITHOUT_EPOLL], [
  AC_ARG_WITH(epoll,
    AS_HELP_STRING([--without-epoll],[do not check for epoll support]),
    [
      if test "$withval" = "yes"; then
        TORRENT_CHECK_EPOLL
      fi
    ], [
        TORRENT_CHECK_EPOLL
    ])
])


AC_DEFUN([TORRENT_CHECK_KQUEUE], [
  AC_MSG_CHECKING(for kqueue support)

  AC_LINK_IFELSE([AC_LANG_SOURCE([
      #include <sys/time.h>  /* Because OpenBSD's sys/event.h fails to compile otherwise. Yeah... */
      #include <sys/event.h>
      int main() {
        int fd = kqueue();
        return 0;
      }
      ])],
    [
      AC_DEFINE(USE_KQUEUE, 1, Use kqueue.)
      AC_MSG_RESULT(yes)
    ], [
      AC_MSG_RESULT(no)
    ])
])


AC_DEFUN([TORRENT_WITH_KQUEUE], [
  AC_ARG_WITH(kqueue,
    AS_HELP_STRING([--with-kqueue],[enable kqueue [[default=no]]]),
    [
        if test "$withval" = "yes"; then
          TORRENT_CHECK_KQUEUE
        fi
    ])
])


AC_DEFUN([TORRENT_WITHOUT_KQUEUE], [
  AC_ARG_WITH(kqueue,
    AS_HELP_STRING([--without-kqueue],[do not check for kqueue support]),
    [
      if test "$withval" = "yes"; then
        TORRENT_CHECK_KQUEUE
      fi
    ], [
        TORRENT_CHECK_KQUEUE
    ])
])


AC_DEFUN([TORRENT_CHECK_FALLOCATE], [
  AC_MSG_CHECKING(for fallocate)

  AC_LINK_IFELSE([AC_LANG_PROGRAM([[#define _GNU_SOURCE
               #include <fcntl.h>
              ]], [[ fallocate(0, FALLOC_FL_KEEP_SIZE, 0, 0); return 0;
              ]])],[
      AC_DEFINE(HAVE_FALLOCATE, 1, Linux's fallocate supported.)
      AC_MSG_RESULT(yes)
    ],[
      AC_MSG_RESULT(no)
    ])
])


AC_DEFUN([TORRENT_CHECK_POSIX_FALLOCATE], [
  AC_MSG_CHECKING(for posix_fallocate)

  AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <fcntl.h>
              ]], [[ posix_fallocate(0, 0, 0);
              ]])],[
      AC_DEFINE(USE_POSIX_FALLOCATE, 1, posix_fallocate supported.)
      AC_MSG_RESULT(yes)
    ],[
      AC_MSG_RESULT(no)
    ])
])


AC_DEFUN([TORRENT_WITH_POSIX_FALLOCATE], [
  AC_ARG_WITH(posix-fallocate,
    AS_HELP_STRING([--with-posix-fallocate],[check for and use posix_fallocate to allocate files]),
    [
      if test "$withval" = "yes"; then
        TORRENT_CHECK_POSIX_FALLOCATE
      fi
    ])
])

AC_DEFUN([TORRENT_WITH_ADDRESS_SPACE], [
  AC_ARG_WITH(address-space,
    AS_HELP_STRING([--with-address-space=MB],[change the default address space size [[default=1024mb]]]),
    [
      if test ! -z $withval -a "$withval" != "yes" -a "$withval" != "no"; then
        AC_DEFINE_UNQUOTED(DEFAULT_ADDRESS_SPACE_SIZE, [$withval])
      else
        AC_MSG_ERROR(--with-address-space requires a parameter.)
      fi
    ],
    [
      AC_CHECK_SIZEOF(long)

      if test $ac_cv_sizeof_long = 8; then
        AC_DEFINE(DEFAULT_ADDRESS_SPACE_SIZE, 4096, Default address space size.)
      else
        AC_DEFINE(DEFAULT_ADDRESS_SPACE_SIZE, 1024, Default address space size.)
      fi
    ])
])

AC_DEFUN([TORRENT_WITH_FASTCGI], [
  AC_ARG_WITH(fastcgi,
    AS_HELP_STRING([--with-fastcgi=PATH],[enable FastCGI RPC support (DO NOT USE)]),
    [
      AC_MSG_CHECKING([for FastCGI (DO NOT USE)])

      if test "$withval" = "no"; then
        AC_MSG_RESULT(no)

      elif test "$withval" = "yes"; then
        CXXFLAGS="$CXXFLAGS"
	LIBS="$LIBS -lfcgi"        

        AC_LINK_IFELSE([AC_LANG_PROGRAM([[ #include <fcgiapp.h>
        ]], [[ FCGX_Init(); ]])],[
          AC_MSG_RESULT(ok)
        ],[
          AC_MSG_RESULT(not found)
          AC_MSG_ERROR(Could not compile FastCGI test.)
        ])

        AC_DEFINE(HAVE_FASTCGI, 1, Support for FastCGI.)

      else
        CXXFLAGS="$CXXFLAGS -I$withval/include"
	LIBS="$LIBS -lfcgi -L$withval/lib"

        AC_LINK_IFELSE([AC_LANG_PROGRAM([[ #include <fcgiapp.h>
        ]], [[ FCGX_Init(); ]])],[
          AC_MSG_RESULT(ok)
        ],[
          AC_MSG_RESULT(not found)
          AC_MSG_ERROR(Could not compile FastCGI test.)
        ])

        AC_DEFINE(HAVE_FASTCGI, 1, Support for FastCGI.)
      fi
    ])
])


AC_DEFUN([TORRENT_WITH_XMLRPC_C], [
  AC_MSG_CHECKING(for XMLRPC-C)

  AC_ARG_WITH(xmlrpc-c,
    AS_HELP_STRING([--with-xmlrpc-c=PATH],[enable XMLRPC-C support]),
  [
    if test "$withval" = "no"; then
      AC_MSG_RESULT(no)

    else
      if test "$withval" = "yes"; then
        xmlrpc_cc_prg="xmlrpc-c-config"
      else
        xmlrpc_cc_prg="$withval"
      fi

      if eval $xmlrpc_cc_prg --version 2>/dev/null >/dev/null; then
        CXXFLAGS="$CXXFLAGS `$xmlrpc_cc_prg --cflags server-util`"
        LIBS="$LIBS `$xmlrpc_cc_prg server-util --libs`"

        AC_LINK_IFELSE([AC_LANG_PROGRAM([[ #include <xmlrpc-c/server.h>
        ]], [[ xmlrpc_registry_new(NULL); ]])],[
          AC_MSG_RESULT(ok)
        ],[
          AC_MSG_RESULT(failed)
          AC_MSG_ERROR(Could not compile XMLRPC-C test.)
        ])

        AC_DEFINE(HAVE_XMLRPC_C, 1, Support for XMLRPC-C.)

      else
        AC_MSG_RESULT(failed)
        AC_MSG_ERROR(Could not compile XMLRPC-C test.)
      fi
    fi

  ],[
    AC_MSG_RESULT(ignored)
  ])
])

AC_DEFUN([TORRENT_WITH_TINYXML2], [
  AC_MSG_CHECKING(for tinyxml2)

  AC_ARG_WITH(xmlrpc-tinyxml2,
    AS_HELP_STRING([--with-xmlrpc-tinyxml2],[enable XMLRPC support via tinyxml2]),
  [
    AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_XMLRPC_TINYXML2, 1, Support for XMLRPC via tinyxml2.)
  ],[
    AC_MSG_RESULT(ignored)
  ])
])

AC_DEFUN([TORRENT_WITH_LUA], [
  AC_ARG_WITH(lua,
    AS_HELP_STRING([--with-lua],[enable LUA support]),
  [
    if test "$withval" = "no"; then
      AC_MSG_RESULT(no)
    else
      AX_PROG_LUA
      AX_LUA_LIBS
      AX_LUA_HEADERS
      AC_DEFINE(HAVE_LUA, 1, Use LUA.)
      AC_DEFINE(LUA_DATADIR, [PACKAGE_DATADIR "/lua"], [LUA data directory])
      LIBS="$LIBS $LUA_LIB"
      CXXFLAGS="$CXXFLAGS $LUA_INCLUDE"
    fi
  ],[
    AC_MSG_RESULT(ignored)
  ])
])

AC_DEFUN([TORRENT_WITH_INOTIFY], [
  AC_LANG_PUSH(C++)

  AC_CHECK_HEADERS([sys/inotify.h mcheck.h])
  AC_MSG_CHECKING([whether sys/inotify.h actually works])

  AC_COMPILE_IFELSE([AC_LANG_SOURCE([
      #include <sys/inotify.h>
      int main(int,const char**) { return (-1 == inotify_init()); }])
    ],[
     AC_DEFINE(HAVE_INOTIFY, 1, [sys/inotify.h exists and works correctly])
     AC_MSG_RESULT(yes)],
    [AC_MSG_RESULT(failed)]
  )

  AC_LANG_POP(C++)
])

AC_DEFUN([TORRENT_CHECK_PTHREAD_SETNAME_NP], [
  AC_CHECK_HEADERS(pthread.h)

  AC_MSG_CHECKING(for pthread_setname_np type)

  AC_LINK_IFELSE([AC_LANG_PROGRAM([[
    #include <pthread.h>
    #include <sys/types.h>
  ]], [[
    pthread_t t;
    pthread_setname_np(t, "foo");
  ]])],[
    AC_DEFINE(HAS_PTHREAD_SETNAME_NP_GENERIC, 1, The function to set pthread name has a pthread_t argumet.)
    AC_MSG_RESULT(generic)
  ],[
    AC_LINK_IFELSE([AC_LANG_PROGRAM([[
      #include <pthread.h>
      #include <sys/types.h>
    ]],[[
      pthread_t t;
      pthread_setname_np("foo");
    ]])],[
      AC_DEFINE(HAS_PTHREAD_SETNAME_NP_DARWIN, 1, The function to set pthread name has no pthread argument.)
      AC_MSG_RESULT(darwin)
    ],[
      AC_MSG_RESULT(no)
    ])
  ])
])

AC_DEFUN([TORRENT_DISABLE_PTHREAD_SETNAME_NP], [
  AC_MSG_CHECKING([for pthread_setname_no])

  AC_ARG_ENABLE(pthread-setname-np,
    AS_HELP_STRING([--disable-pthread-setname-np],[disable pthread_setname_np]),
    [
      if test "$enableval" = "no"; then
        AC_MSG_RESULT(disabled)
      else
        AC_MSG_RESULT(checking)
        TORRENT_CHECK_PTHREAD_SETNAME_NP
      fi
    ], [
      TORRENT_CHECK_PTHREAD_SETNAME_NP
    ]
  )
])
