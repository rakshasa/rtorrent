**Contents**

 * [Installing](#installing)
   * [Building](#building)
   * [Non-blocking hostname lookup in curl](#non-blocking-hostname-lookup-in-curl)
 * [Compilation Help](#compilation-help)
   * [libtorrent.so.5: cannot open shared object file: No such file or directory](#libtorrent-so-5-cannot-open-shared-object-file-no-such-file-or-directory)
   * [No symbol information in backtraces](#no-symbol-information-in-backtraces)
   * [Error checking signedness of mincore parameter](#error-checking-signedness-of-mincore-parameter)
   * [Syntax error when running ./configure](#syntax-error-when-running-configure)
   * [libTorrent not found by ./configure](#libtorrent-not-found-by-configure)
   * [libTorrent version not detected by ./configure](#libtorrent-version-not-detected-by-configure)
   * [Compilation on Centos](#compilation-on-centos)


# Installing


## Building

`./configure; make; make install`


## Non-blocking hostname lookup in curl

rTorrent will block on hostname lookup during tracker requests and http downloads. You can avoid this by compiling libcurl with 'cares' support, though this will be fixed later by using seperate threads within rTorrent.


# Compilation Help

(Source: old trac [Compilation Help](http://web.archive.org/web/20140220165510/http://libtorrent.rakshasa.no/wiki/CompilationHelp) wiki page.)

This section describes some common (non-obvious) problems compiling libTorrent/rTorrent and how to fix them. It does not have general instructions for compiling, see the [Installing](#installing) section for that.

In general, for problems during `./configure` time check the lines of `config.log` before the "Cache variables" section, it should have a more detailed error message to help you diagnose the problem.


## libtorrent.so.5: cannot open shared object file: No such file or directory

Make sure you have `$prefix/lib` in either your `/etc/ld.so.conf` or `LD_LIBRARY_PATH`, where `$prefix` is where you installed libtorrent. To update the ld cache, run `ldconfig`.


## No symbol information in backtraces

When rtorrent crashes, it automatically generates a backtrace (on systems which support this). For these to be useful in fixing the problem, the linker must put symbol information in the executable files (and not just in the debug information section).

This can be accomplished by putting `-rdynamic` in `LDFLAGS` and recompiling. For example, as an argument to the configure script:

`./configure LDFLAGS=-rdynamic`

Add any other options you need, of course. Use `make clean` to force a recompilation if necessary.


## Error checking signedness of mincore parameter

`checking signedness of mincore parameter... configure: error: failed, do *not* attempt fix this with --disable-mincore unless you are running Win32.`

**Problem:** Most likely, no C++ compiler is installed.

**Solution:** Install g++ or any other competent C++ compiler. 

In most cases this error message is very misleading and indicates a general inability to compile C++ code rather than a specific problem with the `signedness` of the `mincore` parameter.


## Syntax error when running ./configure

```
./configure: syntax error near unexpected token `OPENSSL,'
./configure: `      PKG_CHECK_MODULES(OPENSSL, openssl,'
```

**Problem:** autoconf cannot find the pkg-config macros in pkg.m4

**Solution:**
 * Make sure pkg-config is installed, and pkg.m4 is present
 * Make sure autoconf can find pkg.m4
   * If it cannot find it even with pkg-config installed, move pkg.m4 to where aclocal expects it, which is most likely `/usr/share/aclocal`. If you're using MacPorts, you may be running the legacy autoconf instead of the one from MacPorts, in that case ensure that `/opt/local/bin` comes before `/usr/bin` in your PATH environment variable to run the MacPorts autoconf.
 * Run autogen.sh again
 * Run configure again


## libTorrent not found by ./configure

```
checking for STUFF... configure: error: Package requirements (sigc++-2.0 libcurl >= 7.12.0 libtorrent >= 0.11.8) were not met: 
No package 'libtorrent' found
```

**Problem:** libtorrent.pc not installed in a place where pkg-config looks for it.

**Solution:** 

If you have not already compiled and installed libtorrent, do this first.

If this message persists even with libtorrent installed, pkg-config cannot find the libtorrent.pc file. Most likely, you've installed libtorrent in `/usr/local` (or a non-standard location) and pkg-config is not configured to look there. Set the `PKG_CONFIG_PATH` environment variable to where libtorrent.pc may be found, e.g. using bash:

`export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig`

Then run configure again.


## libTorrent version not detected by ./configure

```
checking for STUFF... configure: error: Package requirements (sigc++-2.0 libcurl >= 7.12.0 libtorrent >= 0.12.0) were not met:
Requested 'libtorrent >= 0.12.0' but version of libtorrent is
```

**Problem:** automake is too old 

**Solution:** Upgrade automake to version 1.5 or higher, then run libtorrent's `autogen.sh` and `configure` again, and `make install`.

Alternatively, manually edit `libtorrent.pc` to show the correct version.


## Compilation on Centos

[Howto for installing on Centos](http://markus.revti.com/2009/11/installing-libtorrent-and-rtorrent-on-linux-centos/)

