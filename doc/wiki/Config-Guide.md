# rTorrent Configuration Guide
**Contents**

 * [Basic Configuration](#basic-configuration)
 * [Advanced Use-Cases](#advanced-use-cases)
 * [Advanced Topics](#advanced-topics)
 * [Complete solutions](#complete-solutions)


## Basic Configuration 

See [rTorrent Configuration Template](https://github.com/rakshasa/rtorrent/wiki/CONFIG-Template) for a modern rTorrent configuration that provides a good starting point. The following sub-sections describe some of the essential settings you must have in a common configuration.

### Using a Session Directory

Adding the `session.path.set` command will enable session management, which means the torrent files and status information for all open downloads will be stored in this directory. When restarting rTorrent all torrents previously loaded will be restored. Only one instance of rTorrent should be used with each session directory, though at the moment no locking is done. An empty string will disable the session handling.


### Watching a Directory for Torrents

The client may be configured to check a directory for new torrents and load them. Torrents loaded in this manner will be *tied* to the file's path. This means when the torrent file is deleted the torrent may be stopped (requires additional configuration), and when the item is removed the torrent file is, too. Note that you can untie an item by using the `U` key (which will delete the tied file), and using `^K` also implictly unties an item.

```ini
# Watch directories (add more as you like, but use unique schedule names)
schedule = watch_start,10,10,((load.start,"./watch/start/*.torrent"))
schedule = watch_load,15,10,((load.normal,"./watch/load/*.torrent"))
```

See also the [Watch Directories](https://github.com/rakshasa/rtorrent/wiki/TORRENT-Watch-directories) page.


## Advanced Use-Cases

These pages and the following sections cover information that you don't need when you start out (i.e. read at your leisure), or only apply to a small number of users.

 * [Ratio Handling](https://github.com/rakshasa/rtorrent/wiki/RTorrentRatioHandling)
 * [Tor Proxying](https://github.com/rakshasa/rtorrent/wiki/Tor-based-Proxying-Guide)
 * [Using DHT](https://github.com/rakshasa/rtorrent/wiki/Using-DHT)
 * [Using XMLRPC with rTorrent](https://github.com/rakshasa/rtorrent/wiki/RPC-Setup-XMLRPC)
 * [Performance Tuning](https://github.com/rakshasa/rtorrent/wiki/Performance-Tuning)
 * [Favoring one group of torrents over the rest of them](https://github.com/rakshasa/rtorrent/wiki/Favoring-group-of-torrents)
 * [Auto-Scraping](https://github.com/rakshasa/rtorrent/wiki/Auto-Scraping)

### Manually setting the local IP ###

Using the ''-i <ip>'' flag or ''"ip = <ip>"'' option you may change your ip address that is reported to the tracker. If you have a dynamic ip address then ''"schedule = ip_tick,0,1800,ip=my_address"'' may be used to update the ip address every 30 minutes.

The client may spend as much as 60 seconds trying to contact a UDP tracker, so if you are behind a firewall that blocks the reply packets you should tell the client to skip the UDP tracker. Set "use_udp_trackers = no" in your configuration file or in the command line option.


## Advanced Topics

 * [Logging](https://github.com/rakshasa/rtorrent/wiki/LOG-Logging)
 * [Migration to the 0.9.x command syntax](https://github.com/rakshasa/rtorrent/wiki/RPC-Migration-0.9)
 * [Common Tasks in rTorrent](https://github.com/rakshasa/rtorrent/wiki/Common-Tasks-in-rTorrent)
 * [Choke Groups](https://github.com/rakshasa/rtorrent/wiki/Choke-Groups)
 * [IP filtering](https://github.com/rakshasa/rtorrent/wiki/IP-filtering)


## Complete solutions

* [pimp-my-box](https://github.com/pyroscope/pimp-my-box)
* [Ultimate Torrent Setup](https://github.com/xombiemp/ultimate-torrent-setup/wiki)
* [Automated rTorrent-PS configuration](https://github.com/chros73/rtorrent-ps_setup)
