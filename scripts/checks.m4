AC_DEFUN([TORRENT_CHECK_CURL], [

  AC_CACHE_VAL(my_cv_curl_vers, [
    my_cv_curl_vers=NONE
    dnl check is the plain-text version of the required version
    check="7.12.0"
    dnl check_hex must be UPPERCASE if any hex letters are present
    check_hex="070C00"
 
    AC_MSG_CHECKING([for curl >= $check])
 
    if eval curl-config --version 2>/dev/null >/dev/null; then
      ver=`curl-config --version | sed -e "s/libcurl //g"`
      hex_ver=`curl-config --vernum | tr 'a-f' 'A-F'`
      ok=`echo "ibase=16; if($hex_ver>=$check_hex) $hex_ver else 0" | bc`
 
      if test x$ok != x0; then
        my_cv_curl_vers="$ver"
        AC_MSG_RESULT([$my_cv_curl_vers])

	CURL_CFLAGS=`curl-config --cflags`
	CURL_LIBS=`curl-config --libs`
      else
        AC_MSG_RESULT(FAILED)
        AC_MSG_ERROR([$ver is too old. Need version $check or higher.])
      fi
    else
      AC_MSG_RESULT(FAILED)
      AC_MSG_ERROR([curl-config was not found])
    fi
  ])
])


AC_DEFUN([TORRENT_CHECK_OPENSSL], [
  # first, deal with the user option : set places to be 'search' or the prefix
  AC_ARG_WITH(openssl,
    [  --with-openssl=PATH     Find the OpenSSL header and library in
                          `PATH/include' and `PATH/lib'. If PATH is of the
                          form `HEADER:LIB', then search for header files in
                          HEADER, and the library in LIB.  If you omit the
                          option completely, the configure script will
                          search for OpenSSL in a number of standard
                          places.],
    [
      if test "$withval" = "yes"; then
	PKG_CHECK_MODULES(OPENSSL, openssl,
	                  CXXFLAGS="$CXXFLAGS `pkg-config --cflags openssl`";
                          LIBS="$LIBS -lcrypto `pkg-config --libs-only-L openssl`",
		          AC_MSG_ERROR([Could not find openssl's crypto library], [try --with-openssl=PATH]))

        else
	  CXXFLAGS="$CXXFLAGS -I$withval/include"
	  LIBS="$LIBS -lcrypto -L$withval/lib"
        fi
    ], [
      PKG_CHECK_MODULES(OPENSSL, openssl,
      CXXFLAGS="$CXXFLAGS `pkg-config --cflags openssl`";
      LIBS="$LIBS -lcrypto `pkg-config --libs-only-L openssl`",
      AC_MSG_ERROR([Could not find openssl's crypto library], [try --with-openssl=PATH]))
    ])
])


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
