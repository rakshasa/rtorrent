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

	CURL_CFLAGS="`curl-config --cflags`"
	CURL_LIBS="`curl-config --libs`"
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
                      places.
    ], [
      if test "$withval" = "yes"; then
	PKG_CHECK_MODULES(OPENSSL, openssl,
	                  CXXFLAGS="$CXXFLAGS `pkg-config --cflags openssl`";
                          LIBS="$LIBS -lcrypto `pkg-config --libs-only-L openssl`",
		          AC_MSG_ERROR(Could not find openssl's crypto library, try --with-openssl=PATH))

        else
	  CXXFLAGS="$CXXFLAGS -I$withval/include"
	  LIBS="$LIBS -lcrypto -L$withval/lib"
        fi
    ], [
      PKG_CHECK_MODULES(OPENSSL, openssl,
      CXXFLAGS="$CXXFLAGS `pkg-config --cflags openssl`";
      LIBS="$LIBS -lcrypto `pkg-config --libs-only-L openssl`",
      AC_MSG_ERROR(Could not find openssl's crypto library, try --with-openssl=PATH))
    ])
])
