AC_DEFUN([TORRENT_CHECK_OPENSSL],
  [
    PKG_CHECK_MODULES(OPENSSL, libcrypto,
      CXXFLAGS="$CXXFLAGS $OPENSSL_CFLAGS";
      LIBS="$LIBS $OPENSSL_LIBS")

    AC_DEFINE(USE_OPENSSL, 1, Using OpenSSL.)
    AC_DEFINE(USE_OPENSSL_SHA, 1, Using OpenSSL's SHA1 implementation.)
  ]
)

AC_DEFUN([TORRENT_ARG_OPENSSL],
  [
    AC_ARG_ENABLE(openssl,
      [  --disable-openssl       Don't use OpenSSL's SHA1 implementation.],
      [
        if test "$enableval" = "yes"; then
          TORRENT_CHECK_OPENSSL
        else
          AC_DEFINE(USE_NSS_SHA, 1, Using Mozilla's SHA1 implementation.)
        fi
      ],[
        TORRENT_CHECK_OPENSSL
      ])
  ]
)

AC_DEFUN([TORRENT_ARG_CYRUS_RC4],
  [
    AC_ARG_ENABLE(cyrus-rc4,
      [  --enable-cyrus-rc4=PFX  Use Cyrus RC4 implementation.],
      [
        CXXFLAGS="$CXXFLAGS -I${enableval}/include";
        LIBS="$LIBS -lrc4 -L${enableval}/lib"
        AC_DEFINE(USE_CYRUS_RC4, 1, Using Cyrus RC4 implementation.)
      ])
  ]
)
