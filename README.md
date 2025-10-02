RTorrent BitTorrent Client
========

Introduction
------------

A ncurses-based command line torrent client for high performance. 

To learn how to use rTorrent visit the [Wiki](https://github.com/rakshasa/rtorrent/wiki).

Download the [latest stable release](https://github.com/rakshasa/rtorrent/releases/latest)

Related Projects
----------------

* [https://github.com/rakshasa/rbedit](https://github.com/rakshasa/rbedit): A dependency-free bencode editor.

Donate to rTorrent development
------------------------------

* [Paypal](https://paypal.me/jarisundellno)
* [Patreon](https://www.patreon.com/rtorrent)
* [SubscribeStar](https://www.subscribestar.com/rtorrent)
* Bitcoin: 1MpmXm5AHtdBoDaLZstJw8nupJJaeKu8V8
* Ethereum: 0x9AB1e3C3d8a875e870f161b3e9287Db0E6DAfF78
* Litecoin: LdyaVR67LBnTf6mAT4QJnjSG2Zk67qxmfQ
* Cardano: addr1qytaslmqmk6dspltw06sp0zf83dh09u79j49ceh5y26zdcccgq4ph7nmx6kgmzeldauj43254ey97f3x4xw49d86aguqwfhlte


Help keep rTorrent development going by donating to its creator.


BUILDING
--------

Jump into the github cloned directory

```
cd rtorrent
```

## Install build dependencies

Install [libtorrent](https://github.com/rakshasa/libtorrent) with the same version rTorrent.

Generate configure scripts:

```
autoreconf -ivf
```

Optionally, generate man pages:

```
docbook2man rtorrent.1.xml
```

Man pages output to "doc/rtorrent.1".

RTorrent follows the development of [libtorrent](https://github.com/rakshasa/libtorrent) closely, and thus the versions must be in sync.

## USAGE

Refer to User Guide: https://github.com/rakshasa/rtorrent/wiki/User-Guide

## LICENSE

GNU GPL, see COPYING. "libtorrent/src/utils/sha_fast.{cc,h}" is
originally from the Mozilla NSS and is under a triple license; MPL,
LGPL and GPL. An exception to non-NSS code has been added for linking to OpenSSL as requested by Debian, though the author considers that library to be part of the Operative System and thus linking is allowed according to the GPL.

Use whatever fits your purpose, the code required to compile with
Mozilla's NSS implementation of SHA1 has been retained and can be
compiled if the user wishes to avoid using OpenSSL.

## DEPENDENCIES

* libcurl >= 7.12.0
* libtorrent = (same version)
* ncurses

## BUILD DEPENDENCIES

* libtoolize
* aclocal
* autoconf
* autoheader
* automake
