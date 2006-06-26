#! /bin/sh

echo aclocal...
(aclocal --version) < /dev/null > /dev/null 2>&1 || {
    echo aclocal not found
    exit 1
}

aclocal -I ./scripts -I . ${ACLOCAL_FLAGS}

echo autoheader...
(autoheader --version) < /dev/null > /dev/null 2>&1 || {
    echo autoheader not found
    exit 1
}

autoheader

echo -n "libtoolize... "
if ( (glibtoolize --version) < /dev/null > /dev/null 2>&1 ); then
    echo "using glibtoolize"
    glibtoolize --automake --copy --force

elif ( (libtoolize --version) < /dev/null > /dev/null 2>&1 ) ; then
    echo "using libtoolize"
    libtoolize --automake --copy --force

else
    echo "libtoolize nor glibtoolize not found"
    exit 1
fi

echo automake...
(automake --version) < /dev/null > /dev/null 2>&1 || {
    echo automake not found
    exit 1
}

automake --add-missing --copy --gnu

echo autoconf...
(autoconf --version) < /dev/null > /dev/null 2>&1 || {
    echo autoconf not found
    exit 1
}

autoconf

echo ready to configure

exit 0
