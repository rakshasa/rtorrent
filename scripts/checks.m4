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
      use_epoll=yes

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
      use_kqueue=yes

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


AC_DEFUN([TORRENT_WITH_ADDRESS_SPACE], [
  AC_ARG_WITH(address-space,
    AS_HELP_STRING([--with-address-space=MB],[change the default address space size [default=1024mb-or-32768mb]]),
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
        AC_DEFINE(DEFAULT_ADDRESS_SPACE_SIZE, 32768, Default address space size.)
      else
        AC_DEFINE(DEFAULT_ADDRESS_SPACE_SIZE, 1024, Default address space size.)
      fi
    ])
])


AC_DEFUN([TORRENT_CHECK_ATOMIC], [
  AC_MSG_CHECKING([whether 64-bit atomic operations require -latomic])
  AC_LANG_PUSH(C++)

  AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <cstdint>
                                    #include <atomic>]],
                                    [[std::atomic<uint64_t> x(0);
                                    return x.load();]])],
    [
      AC_MSG_RESULT([no])
      ATOMIC_LIBS=""
    ],
    [
      save_LIBS="$LIBS"
      LIBS="$LIBS -latomic"

      AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <cstdint>
                                        #include <atomic>]],
                                        [[std::atomic<uint64_t> x(0);
                                        return x.load();]])],
        [
          AC_MSG_RESULT([yes])
          ATOMIC_LIBS="-latomic"
        ],
        [
          AC_MSG_RESULT([unsupported])
          AC_MSG_ERROR([Compiler target lacks proper 64-bit atomic support.])
        ])

      LIBS="$save_LIBS"
    ])

  AC_LANG_POP(C++)
  AC_SUBST([ATOMIC_LIBS])
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

      # 1. Override AX_LUA_LIBS default crash behavior 
      AX_LUA_LIBS([have_lua_libs=yes], [have_lua_libs=no])

      # 2. Override AX_LUA_HEADERS default crash behavior
      AX_LUA_HEADERS([have_lua_headers=yes], [have_lua_headers=no])

      # 3. Only inject if both checks completely pass
      if test "x$have_lua_libs" = "xyes" && test "x$have_lua_headers" = "xyes"; then
        AC_DEFINE(HAVE_LUA, 1, Use LUA.)
        AC_DEFINE(LUA_DATADIR, [PACKAGE_DATADIR "/lua"], [LUA data directory])
        LIBS="$LIBS $LUA_LIB"
        CXXFLAGS="$CXXFLAGS $LUA_INCLUDE"
      else
        # Throw fatal error ONLY if user strictly ran --with-lua=yes
        if test "$withval" = "yes"; then
          AC_MSG_ERROR([Lua support explicitly requested, but compatible Lua 5.3 libs/headers were not found.])
        else
          AC_MSG_WARN([Lua 5.3 libs or headers missing. Proceeding without Lua support.])
        fi
      fi
    fi
  ],[
    AC_MSG_RESULT(ignored)
  ])
])


AC_DEFUN([TORRENT_WITH_INOTIFY], [
  AC_LANG_PUSH(C++)

  AC_CHECK_HEADERS([sys/inotify.h])
  AC_MSG_CHECKING([whether sys/inotify.h actually works])

  AC_COMPILE_IFELSE([AC_LANG_SOURCE([
      #include <sys/inotify.h>
      int main(int,const char**) { return (-1 == inotify_init()); }])
    ],[
     AC_DEFINE(USE_INOTIFY, 1, [sys/inotify.h exists and works correctly])
     AC_MSG_RESULT(yes)],
    [AC_MSG_RESULT(failed)]
  )

  AC_LANG_POP(C++)
])


AC_DEFUN([TORRENT_CHECK_PTHREAD_SETNAME_NP], [
  AC_CHECK_HEADERS(pthread.h)

  AC_MSG_CHECKING(for pthread_setname_np type)

  AC_LINK_IFELSE([AC_LANG_PROGRAM([[
    #define _GNU_SOURCE
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


AC_DEFUN([TORRENT_CHECK_POSIX_SPAWN_ADDCLOSEFROM_NP], [
  AC_MSG_CHECKING(for posix_spawn_file_actions_addclosefrom_np)

  AC_LINK_IFELSE([AC_LANG_PROGRAM([[
    #define _GNU_SOURCE
    #include <spawn.h>
  ]], [[
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_addclosefrom_np(&actions, 3);
  ]])],[
    AC_DEFINE(HAVE_POSIX_SPAWN_FILE_ACTIONS_ADDCLOSEFROM_NP, 1, [Define if posix_spawn_file_actions_addclosefrom_np is available.])
    AC_MSG_RESULT(yes)
  ],[
    AC_MSG_RESULT(no)
  ])
])


AC_DEFUN([TORRENT_WITH_SYSTEMD], [
  AC_ARG_WITH(systemd,
    AS_HELP_STRING([--with-systemd],[enable systemd socket activation support [[default=no]]]),
    [
      if test "$withval" = "yes"; then
        PKG_CHECK_MODULES([SYSTEMD], [libsystemd],
          [
            CXXFLAGS="$CXXFLAGS $SYSTEMD_CFLAGS"
            LIBS="$LIBS $SYSTEMD_LIBS"
            AC_DEFINE(HAVE_SYSTEMD, 1, [Support for systemd socket activation.])
          ],
          [AC_MSG_ERROR([libsystemd not found. Install libsystemd-dev (or the equivalent for your distribution).])])
      fi
    ])
])


AC_DEFUN([TORRENT_WITHOUT_NCURSES], [
  AC_ARG_WITH([ncurses],
    [AS_HELP_STRING([--without-ncurses], [build without ncurses (daemon-only mode)])],
    [with_ncurses=$withval],
    [with_ncurses=yes])

  if test "x$with_ncurses" = xno; then
    AC_DEFINE([HAVE_NO_NCURSES], [1], [Define to 1 if building without ncurses])
    CURSES_LIBS=""
    CURSES_CFLAGS=""
    CURSES_LIB=""
  fi

  AM_CONDITIONAL([NO_NCURSES], [test "x$with_ncurses" = xno])
])
