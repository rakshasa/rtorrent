# Using XMLRPC with rTorrent

What you need:

* http://python.ca/scgi/ for Apache, Lighttpd should have this built-in.
* http://xmlrpc-c.sourceforge.net/ 1.00 or later, 1.07 or later for 64bit integer support.

Configure rtorrent with the `--with-xmlrpc-c` flag. Then add appropriate configuration, according used web-server.

### Apache

**httpd.conf**:
```ini
SCGIMount /RPC2 127.0.0.1:5000
```

**rtorrent.rc**:
```ini
scgi_port = localhost:5000
```

### Lighttpd:

**rtorrent.rc**:
```ini
scgi_local = /home/user/rtorrent/rpc.socket

# Set correct access rights to the socket file ----------+
# so both, lighttpd and rtorrent, could perform          |
# read-write operations                                  v
schedule = scgi_permission,0,0,"execute.nothrow=chmod,\"g+w,o=\",/home/user/rtorrent/rpc.socket"
```

**lighttpd.conf**:
```ini
server.modules += ( "mod_scgi" )
scgi.server = (
                "/RPC2" =>
                  ( "127.0.0.1" =>
                    (
                      "socket" => "/home/user/rtorrent/rpc.socket",
                      "check-local" => "disable",
                      "disable-time" => 0,  # don't disable scgi if connection fails
                    )
                  )
              )
```

### Nginx:

**rtorrent.rc**:
```ini
scgi_port = localhost:5000
```

**nginx.conf**:
```
location /RPC2 {
  scgi_pass   127.0.0.1:5000;
  include     scgi_vars;
  scgi_var    SCRIPT_NAME  /RPC2;
}
```
or (on ubuntu server 14.04)
```
http {
  server {
    listen 0.0.0.0:8008;
    server_name ngnix-rtorrent;
    access_log /var/log/nginx/rtorrent.access_log;
    error_log /var/log/nginx/rtorrent.error_log;

    location /RPC2 {
      scgi_pass   127.0.0.1:5000;
      include     scgi_params;
    }
  }
}
```

Do not forget, that on http://localhost:8008/RPC2 you will not see anything (through in rtorrent.error_log you will see something as `upstream prematurely closed connection while reading response header from upstream, client: 192.168.93.104, server: ngnix-rtorrent, request: "GET /RPC2 HTTP/1.1", upstream: "scgi://127.0.0.1:5000", host: "192.168.93.242:8008"`. It is xmlrpc, not web service. You can test it by xmlrpc (see later))

## Other notes

If any of your downloads have non-ascii characters in the filenames, you must also set the following in rtorrent.rc to force rtorrent to use the UTF-8 encoding. The XMLRPC standard requires UTF-8 replies, and rtorrent presently has no facilities to convert between encodings so it might generate invalid replies otherwise.

```
encoding_list = UTF-8
```

The web server will now route xmlrpc requests to rtorrent, which is listening only on connections from the local machine or on the local socket file. Also make sure the /RPC2 location is properly protected.

To make it accessible from anywhere, use `scgi_port = :5000`. This is however not recommend as rtorrent has no access control, which means the http server is responsible for handling that. Anyone who can send rtorrent xmlrpc commands is likely to have the ability to execute code with the privileges of the user running rtorrent.

You may also use `scgi_local = /foo/bar` to create a local domain socket, which supports file permissions. Set the rw permissions of the directory the socket will reside in to only allow the necessary processes. This is the recommended way of using XMLRPC with rtorrent, though not all http servers support local domain sockets for scgi.

## Test and usage

Access the XMLRPC interface using any XMLRPC-capable client. For example, using the xmlrpc utility that comes with xmlrpc-c:

```
 > # To list all the xmlrpc methods rtorrent supports.
 > xmlrpc localhost system.listMethods

 > # Get max upload rate.
 > xmlrpc localhost get_upload_rate ""

 > # Set upload rate, exact 100000 bytes.
 > xmlrpc localhost set_upload_rate "" i/100000

 > # Set upload rate, 100kb.
 > xmlrpc localhost set_upload_rate "" 100k

 > # See list of downloads in "main" view
 > xmlrpc localhost download_list ""

 > # See list of downloads in "started" view
 > xmlrpc localhost download_list "" started

 > # Get uploaded bytes for specific download by info-hash
 > xmlrpc localhost d.get_up_total e66e7012b8346271009110ac38f91bc0ad8ce281

 > # Change the directory for a specific download.
 > xmlrpc localhost d.set_directory 91A2DF0C9288BC4C5D03EC8D8C26B4CF95A4DBEF foo/bar/

 > # Size of the first file in a specific download.
 > xmlrpc localhost f.get_size_bytes 91A2DF0C9288BC4C5D03EC8D8C26B4CF95A4DBEF i/0
```

It supports both single strings akin to what the option file accepts, and proper xmlrpc integer, string and lists.

See the man page and the rtorrent/src/command_* source files for more details on what parameters some of the commands take.

## Targets

Note that all commands now require a target, even if it is an empty string.

```
 > xmlrpc localhost f.get_size_bytes 91A2DF0C9288BC4C5D03EC8D8C26B4CF95A4DBEF i/3
 > xmlrpc localhost f.get_size_bytes 91A2DF0C9288BC4C5D03EC8D8C26B4CF95A4DBEF "3"
 > xmlrpc localhost f.get_size_bytes 91A2DF0C9288BC4C5D03EC8D8C26B4CF95A4DBEF:f3
 > xmlrpc localhost p.get_url        91A2DF0C9288BC4C5D03EC8D8C26B4CF95A4DBEF:p0
```

The first and second example passes the index of the file as an integer and string respectively. The third example uses a more compact syntax that contains both the info hash, type and index in the same string.