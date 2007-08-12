AC_DEFUN([TORRENT_CHECK_XFS], [
  AC_MSG_CHECKING(for XFS support)

  AC_COMPILE_IFELSE(
    [[#include <xfs/libxfs.h>
      #include <sys/ioctl.h>
      int main() {
        struct xfs_flock64 l;
        ioctl(0, XFS_IOC_RESVSP64, &l);
        return 0;
      }
    ]],
    [
      AC_DEFINE(USE_XFS, 1, Use XFS filesystem stuff.)
      AC_MSG_RESULT(yes)
    ], [
      AC_MSG_RESULT(no)
    ])
])


AC_DEFUN([TORRENT_WITHOUT_XFS], [
  AC_ARG_WITH(xfs,
    [  --without-xfs           Do not check for XFS filesystem support],
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
    [  --with-xfs           Check for XFS filesystem support],
    [
      if test "$withval" = "yes"; then
        TORRENT_CHECK_XFS
      fi
    ])
])


AC_DEFUN([TORRENT_CHECK_EPOLL], [
  AC_MSG_CHECKING(for epoll support)

  AC_COMPILE_IFELSE(
    [[#include <sys/epoll.h>
      int main() {
        int fd = epoll_create(100);
        return 0;
      }
    ]],
    [
      AC_DEFINE(USE_EPOLL, 1, Use epoll.)
      AC_MSG_RESULT(yes)
    ], [
      AC_MSG_RESULT(no)
    ])
])


AC_DEFUN([TORRENT_WITHOUT_EPOLL], [
  AC_ARG_WITH(epoll,
    [  --without-epoll         Do not check for epoll support.],
    [
      if test "$withval" = "yes"; then
        TORRENT_CHECK_EPOLL
      fi
    ], [
        TORRENT_CHECK_EPOLL
    ])
])


AC_DEFUN([TORRENT_WITH_KQUEUE], [
  AC_ARG_WITH(kqueue,
    [  --with-kqueue           enable kqueue. [[default=no]]],
    [
        if test "$withval" = "yes"; then
            AC_DEFINE(USE_KQUEUE, 1, Enable kqueue.)
        fi
    ])
])


AC_DEFUN([TORRENT_WITHOUT_VARIABLE_FDSET], [
  AC_ARG_WITH(variable-fdset,

    [  --without-variable-fdset       do not use non-portable variable sized fd_set's.],
    [
      if test "$withval" = "yes"; then
        AC_DEFINE(USE_VARIABLE_FDSET, 1, defined when we allow the use of fd_set's of any size)
      fi
    ], [
      AC_DEFINE(USE_VARIABLE_FDSET, 1, defined when we allow the use of fd_set's of any size)
    ])
])


AC_DEFUN([TORRENT_CHECK_POSIX_FALLOCATE], [
  AC_MSG_CHECKING(for posix_fallocate)

  AC_COMPILE_IFELSE(
    [[#include <fcntl.h>
      int main() {
	posix_fallocate(0, 0, 0);
        return 0;
      }
    ]],
    [
      AC_DEFINE(USE_POSIX_FALLOCATE, 1, posix_fallocate supported.)
      AC_MSG_RESULT(yes)
    ], [
      AC_MSG_RESULT(no)
    ])
])


AC_DEFUN([TORRENT_WITH_POSIX_FALLOCATE], [
  AC_ARG_WITH(posix-fallocate,
    [  --with-posix-fallocate  Check for and use posix_fallocate to allocate files.],
    [
      if test "$withval" = "yes"; then
        TORRENT_CHECK_POSIX_FALLOCATE
      fi
    ])
])

AC_DEFUN([TORRENT_WITHOUT_NCURSESW], [
  AC_ARG_WITH(ncursesw,
    [  --without-ncursesw      Don't try to use wide char ncurses.],
    [
      if test "$withval" = "no"; then
	AC_SEARCH_LIBS(add_wch, ncursesw,
	  AC_DEFINE(HAVE_NCURSESW, 1, defined if ncurses wide character support is available),
	  AC_SEARCH_LIBS(wbkgdset, ncurses curses,,echo "*** The ncurses library is required!";exit 1)
        )
      else
	AC_SEARCH_LIBS(wbkgdset, ncurses curses,,echo "*** The ncurses library is required!";exit 1)
      fi
    ],[
      AC_SEARCH_LIBS(add_wch, ncursesw,
	AC_DEFINE(HAVE_NCURSESW, 1, defined if ncurses wide character support is available),
	AC_SEARCH_LIBS(wbkgdset, ncurses curses,,echo "*** The ncurses library is required!";exit 1)
      )
    ])
])

AC_DEFUN([TORRENT_CHECK_STATVFS], [
  AC_CHECK_HEADERS(sys/vfs.h sys/statvfs.h sys/statfs.h)

  AC_MSG_CHECKING(for statvfs)

  AC_TRY_LINK(
    [
      #if HAVE_SYS_VFS_H
      #include <sys/vfs.h>
      #endif
      #if HAVE_SYS_STATVFS_H
      #include <sys/statvfs.h>
      #endif
      #if HAVE_SYS_STATFS_H
      #include <sys/statfs.h>
      #endif
    ],[
      struct statvfs s; fsblkcnt_t c;
      statvfs("", &s);
      fstatvfs(0, &s);
    ],
    [
      AC_DEFINE(FS_STAT_FD, [fstatvfs(fd, &m_stat) == 0], Function to determine filesystem stats from fd)
      AC_DEFINE(FS_STAT_FN, [statvfs(fn, &m_stat) == 0], Function to determine filesystem stats from filename)
      AC_DEFINE(FS_STAT_STRUCT, [struct statvfs], Type of second argument to statfs function)
      AC_DEFINE(FS_STAT_SIZE_TYPE, [unsigned long], Type of block size member in stat struct)
      AC_DEFINE(FS_STAT_COUNT_TYPE, [fsblkcnt_t], Type of block count member in stat struct)
      AC_DEFINE(FS_STAT_BLOCK_SIZE, [(m_stat.f_frsize)], Determine the block size)
      AC_MSG_RESULT(ok)
      have_stat_vfs=yes
    ],
    [
      AC_MSG_RESULT(no)
      have_stat_vfs=no
    ])
])

AC_DEFUN([TORRENT_CHECK_STATFS], [
  AC_CHECK_HEADERS(sys/statfs.h sys/param.h sys/mount.h)

  AC_MSG_CHECKING(for statfs)

  AC_TRY_LINK(
    [
      #if HAVE_SYS_STATFS_H
      #include <sys/statfs.h>
      #endif
      #if HAVE_SYS_PARAM_H
      #include <sys/param.h>
      #endif
      #if HAVE_SYS_MOUNT_H
      #include <sys/mount.h>
      #endif
    ],[
      struct statfs s;
      statfs("", &s);
      fstatfs(0, &s);
    ],
    [
      AC_DEFINE(FS_STAT_FD, [fstatfs(fd, &m_stat) == 0], Function to determine filesystem stats from fd)
      AC_DEFINE(FS_STAT_FN, [statfs(fn, &m_stat) == 0], Function to determine filesystem stats from filename)
      AC_DEFINE(FS_STAT_STRUCT, [struct statfs], Type of second argument to statfs function)
      AC_DEFINE(FS_STAT_SIZE_TYPE, [long], Type of block size member in stat struct)
      AC_DEFINE(FS_STAT_COUNT_TYPE, [long], Type of block count member in stat struct)
      AC_DEFINE(FS_STAT_BLOCK_SIZE, [(m_stat.f_bsize)], Determine the block size)
      AC_MSG_RESULT(ok)
    ],
    [
      AC_MSG_RESULT(no)
    ])
])

AC_DEFUN([TORRENT_DISABLED_STATFS], [
      AC_DEFINE(FS_STAT_FD, [(errno = ENOSYS) == 0], Function to determine filesystem stats from fd)
      AC_DEFINE(FS_STAT_FN, [(errno = ENOSYS) == 0], Function to determine filesystem stats from filename)
      AC_DEFINE(FS_STAT_STRUCT, [struct {blocksize_type  f_bsize; blockcount_type f_bavail;}], Type of second argument to statfs function)
      AC_DEFINE(FS_STAT_SIZE_TYPE, [int], Type of block size member in stat struct)
      AC_DEFINE(FS_STAT_COUNT_TYPE, [int], Type of block count member in stat struct)
      AC_DEFINE(FS_STAT_BLOCK_SIZE, [(4096)], Determine the block size)
      AC_MSG_RESULT(No filesystem stats available)
])

AC_DEFUN([TORRENT_WITHOUT_STATVFS], [
  AC_ARG_WITH(statvfs,
    [  --without-statvfs       Don't try to use statvfs to find free diskspace.],
    [
      if test "$withval" = "yes"; then
        TORRENT_CHECK_STATVFS
      else
        have_stat_vfs=no
      fi
    ],
    [
      TORRENT_CHECK_STATVFS
    ])
])

AC_DEFUN([TORRENT_WITHOUT_STATFS], [
  AC_ARG_WITH(statfs,
    [  --without-statfs        Don't try to use statfs to find free diskspace.],
    [
      if test "$have_stat_vfs" = "no"; then
        if test "$withval" = "yes"; then
          TORRENT_CHECK_STATFS
        else
          TORRENT_DISABLED_STATFS
        fi
      fi
    ],
    [
      if test "$have_stat_vfs" = "no"; then
        TORRENT_CHECK_STATFS
        if test "$have_stat_vfs" = "no"; then
          TORRENT_DISABLED_STATFS
        fi
      fi
    ])
])

AC_DEFUN([TORRENT_WITH_ADDRESS_SPACE], [
  AC_ARG_WITH(address-space,
    AC_HELP_STRING([--with-address-space=MB], [Change the default address space size, default 1024 MB.]),
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
    [  --with-fastcgi=PATH      Enable FastCGI RPC support. (DO NOT USE)],
    [
      AC_MSG_CHECKING([for FastCGI (DO NOT USE)])

      if test "$withval" = "no"; then
        AC_MSG_RESULT(no)

      elif test "$withval" = "yes"; then
        CXXFLAGS="$CXXFLAGS"
	LIBS="$LIBS -lfcgi"        

        AC_TRY_LINK(
        [ #include <fcgiapp.h>
        ],[ FCGX_Init(); ],
        [
          AC_MSG_RESULT(ok)
        ],
        [
          AC_MSG_RESULT(not found)
          AC_MSG_ERROR(Could not compile FastCGI test.)
        ])

        AC_DEFINE(HAVE_FASTCGI, 1, Support for FastCGI.)

      else
        CXXFLAGS="$CXXFLAGS -I$withval/include"
	LIBS="$LIBS -lfcgi -L$withval/lib"

        AC_TRY_LINK(
        [ #include <fcgiapp.h>
        ],[ FCGX_Init(); ],
        [
          AC_MSG_RESULT(ok)
        ],
        [
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
  [  --with-xmlrpc-c=PATH     Enable XMLRPC-C support.],
  [
    if test "$withval" = "no"; then
      AC_MSG_RESULT(no)

    else
      if eval xmlrpc-c-config --version 2>/dev/null >/dev/null; then
        CXXFLAGS="$CXXFLAGS `xmlrpc-c-config --cflags server-util`"
        LIBS="$LIBS -lxmlrpc_server `xmlrpc-c-config --libs server-util`"

        AC_TRY_LINK(
        [ #include <xmlrpc-c/server.h>
        ],[ xmlrpc_registry_new(NULL); ],
        [
          AC_MSG_RESULT(ok)
        ], [
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
