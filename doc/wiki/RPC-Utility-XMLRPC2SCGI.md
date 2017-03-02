Utility XMLRPC2SCGI
===================

* http://libtorrent.rakshasa.no/downloads/xmlrpc2scgi.py

Synopsis
--------

```
xmlrpc2scgi.py [-p] scgi://localhost:1234 command target [arg ...]
xmlrpc2scgi.py [-p] /path/to/unix_socket command target [arg ...]
```

Command
-------

Types
-----

### ""

Empty type.

### s/foobar "s/foo bar"

Explicit string type.

### i/1234

Explicit integer type.

Target
------

The target must always be defined, even for commands that do not require any. In such cases "" should be used.

### ""

No target.

### s/00112233445566778899AABBCCDDEEFF00112233

A download is set as the target using the hexadecimal of its info hash.

### s/00112233445566778899AABBCCDDEEFF00112233:f123

A file target with its index.

### s/00112233445566778899AABBCCDDEEFF00112233:p00112233445566778899AABBCCDDEEFF00112233

A peer target with its peer id as a hexadecimal.

### s/00112233445566778899AABBCCDDEEFF00112233:t123

A tracker target with its index.


Examples
--------

Using the _xmlrpc2scgi.py_ script one can send SCGI commands directly to rTorrent.

```
~# ./xmlrpc2scgi.py -p 'scgi://localhost:33000' d.name s/1BEB405E5066392CDAE96EEF9F82F7D83C2343E9
Gotan Project - Lunatico (2006)
```

Without the '-p' flag it will output the SCGI reply without prettifying:

```
~# ./xmlrpc2scgi.py 'scgi://localhost:33000' d.name s/1BEB405E5066392CDAE96EEF9F82F7D83C2343E9
<?xml version="1.0" encoding="UTF-8"?>
<methodResponse>
<params>
<param><value><string>Gotan Project - Lunatico (2006)</string></value></param>
</params>
</methodResponse>
```
