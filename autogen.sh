#!/bin/sh
set -eu

run() {
    printf '%s' "$1" >&2
    if ! command -v "$1" >/dev/null 2>&1; then
        echo ' not found in PATH' >&2
        return 1
    fi
    echo '...' >&2
    "$@"
}

run aclocal -I ./scripts -I .
run autoheader
run glibtoolize --automake --copy --force || run libtoolize --automake --copy --force
run automake --add-missing --copy --gnu
run autoconf
echo 'ready to configure'
