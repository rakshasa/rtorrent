Welcome to rTorrent's Tor-based HTTP tracker proxying guide.

In this human-readable walkthrough, we reconfigure rTorrent to route all HTTP
tracker traffic – including both DNS-based hostname lookups and IP torrent
traffic – through [Tor](https://www.torproject.org).

## Why Tor?

Tor is an open-source anonymity network with extensive cross-platform support,
an active development and volunteer community, ongoing academic research, and
(most relevantly) an application-independent SOCKS interface. _Any_ application
with SOCKS proxy support may route arbitrary data through the anonymizing Tor
network without constraints, censorship, or surveillance.

Tor also [officially
discourages](https://blog.torproject.org/blog/bittorrent-over-tor-isnt-good-idea)
torrent traffic, although there's _not_ particularly much Tor developers can do
about that short of brute-force packing shaping (and probably breaking onion
encapsulation in the process).

In other words, Tor is perfect in all but fundamentalist ideology. And the ideology
is safely ignorable.

## What's the Catch?

To preserve anonymity, **this guide requires disabling _all_ rTorrent features
requiring UDP support.** This includes:

* UDP tracker support.
* [Distributed hash table
  (DHT)](https://en.wikipedia.org/wiki/Distributed_hash_table) support.
* [Peer exchange (PEX)](https://en.wikipedia.org/wiki/Peer_exchange) support.

Nothing's perfect. Especially nothing free as in both [beer and
speech](https://en.wikipedia.org/wiki/Gratis_versus_libre). See the 
[concluding section](#theres-got-to-be-another-way) for alternative approaches.

## I Want My UDP Support and I Want It Now

**Too bad.** At least, until [rakshasa](https://github.com/rakshasa) and company
submit a pull request addressing this [long-standing
issue](https://github.com/rakshasa/rtorrent/issues/105) – and possibly not even
then.

rTorrent seamlessly supports both
[SOCKS4](https://en.wikipedia.org/wiki/Socks_4a#SOCKS4)- and
[SOCKS4a](https://en.wikipedia.org/wiki/Socks_4a#SOCKS4a)-compliant HTTP proxies
"out of the box." rTorrent does _not_
[appear](https://github.com/rakshasa/rtorrent/issues/105) to currently support
[SOCKS5](https://en.wikipedia.org/wiki/Socks_4a#SOCKS5)-compliant HTTP and UDP
proxies. Since SOCKS4 and SOCKS4a proxy only HTTP rather than UDP connections,
enabling HTTP proxying _and_ UDP support in rTorrent will reliably expose your
IP address to malicious middlemen (e.g., [copyright
trolls](https://en.wikipedia.org/wiki/Copyright_troll)).

To safeguard user anonymity, this guide disables UDP support altogether.
Somethin' is better than nothin', right?

## Can We Get Started, Already?

**Let's do this.** In order, the following instructions:

1. Install either:
  * The [Tor Browser
    Bundle](https://www.torproject.org/projects/torbrowser.html.en).
    _**(Strongly recommended.)**_
  * Tor as a headless system daemon.
1. Install [privoxy](http://www.privoxy.org).
1. Configure privoxy to forward all traffic to a local HTTP Tor proxy.
1. Configure rTorrent to forward all traffic to a local SOCKS4a privoxy proxy.

## Step 1: Tor

Unless you have a compelling reason to install Tor as a headless system daemon
(e.g., you run a Tor relay or exit node – in which case, muchas thanks!), this
guide strongly recommends installing the [Tor Browser
Bundle (TBB)](https://www.torproject.org/projects/torbrowser.html.en).

Doing so is trivial and requires no further configuration or customization. In
brief:

1. **Install the
   [TBB](https://www.torproject.org/projects/torbrowser.html.en).**
1. **Run the installed TBB.** On startup, TBB implicitly starts Tor as a
   userspace daemon in the background. On shutdown, TBB implicitly stops this
   userspace daemon – thus stopping all torrents in rTorrent configured to
   proxy torrents through Tor. This implies that TBB _must_ remain open while
   rTorrent is open. Closing TBB closes Tor and hence all torrents in rTorrent.
1. **Verify that the Tor HTTP proxy is listening on the expected port.** The
   default TBB proxy port is 9150; the default non-TBB (i.e., headless system
   daemon) proxy port is 9050. For simplicity, these instructions assume the
   default TBB proxy port of 9150. Under non-Windows systems with
   [netstat](https://en.wikipedia.org/wiki/netstat) installed:
   1. Run:

            $ sudo netstat -atnp | grep tor

   1. A line resembling the following should be output:

            tcp        0      0 127.0.0.1:9150          0.0.0.0:*               LISTEN      32359/tor

Congratulations, salutations, and libations for all. Tor is up!

## Step 2: Privoxy

[Privoxy](http://www.privoxy.org) is a non-caching web proxy whose configuration
file exceeds 2,000 lines in length. It's _a little_ complex. While Tor itself
provides a SOCKS5 proxy, this proxy typically leaks DNS hostname lookups, blocks
default torrent ports, appears to unceasingly hate torrents in general, _and_ is
unsupported by rTorrent – which, again, only supports SOCKS4 and SOCKS4a proxies.

We will now install, configure, and start privoxy as a headless system daemon
under Linux, a headless user daemon under OS X, and a GUI-driven user daemon
under Windows.

1. **Install Privoxy.** Under:
   * Debian-based Linux distributions (e.g., Ubuntu, Mint, Debian), run:

            $ sudo apt-get install privoxy

   * Gentoo-based Linux distributions (e.g., Calculate, Sabayon, Gentoo), run:

            $ sudo emerge privoxy

   * OS X, download and install the [most recent stable OS X
     release](https://sourceforge.net/projects/ijbswa/files/Macintosh%20%28OS%20X%29)
     specific to your machine architecture (e.g., [`Privoxy 3.0.24 64
     bit.pkg`](https://sourceforge.net/projects/ijbswa/files/Macintosh%20%28OS%20X%29/3.0.24%20%28stable%29/Privoxy%203.0.24%2064%20bit.pkg/download)
     for 64-bit machines).
   * Windows, download and install the [most recent stable Windows
     release](https://sourceforge.net/projects/ijbswa/files/Macintosh%20%28OS%20X%29)
     (e.g.,
     [`privoxy_setup_3_0_24.exe`](https://sourceforge.net/projects/ijbswa/files/Win32/3.0.24%20%28stable%29/privoxy_setup_3_0_24.exe/download)).
1. **Configure Privoxy.**
   1. **Find the installed Privoxy configuration file.** Under:
      * Most Linux distributions, this file resides at `/etc/privoxy/config`.
      * OS X, this file typically resides at `/Applications/Privoxy.app/config`.
      * Windows, this file typically resides at
        `C:\Program Files/Privoxy\config.txt`.
   1. **Edit this file.**
      1. **Search this file for the `listen-address` option.** Configure Privoxy
         to listen on the default Privoxy port 8118. Add the following
         uncommented line under this option's commentary:

                listen-address  127.0.0.1:8118

      1. **Search this file for the `forward` option.** Configure Privoxy to act
         as a forward SOCKS4a proxy for Tor (i.e., to relay all traffic on the
         default Privoxy port 8118 to and from the default TBB HTTP proxy port
         9150). SOCKS4a is strongly recommended over SOCKS4, which fails to
         proxy (and hence leaks) DNS hostname lookups. Add the following
         uncommented line under this option's commentary:

                forward-socks4a   /               127.0.0.1:9150 .

      1. **_(Optional)_ Search the Privoxy configuration file for the `debug` option.** By
         default, Privoxy disables logging. Consider configuring Privoxy to log
         a small number of terse status messages by adding the following
         uncommented lines under this option's commentary:

                debug     1 # Log the destination for each request Privoxy let through. See also debug 1024.
                debug  1024 # Actions that are applied to all sites and maybe overruled later on.
                debug  4096 # Startup banner and warnings
                debug  8192 # Non-fatal errors

      1. **_(Optional)_ Search the Privoxy configuration file for the `logdir` and `logfile`
         options.** Both should be uncommented by default and require no
         changes. The `logdir` option provides the absolute path of the
         directory containing all Privoxy logfiles. The `logfile` option
         provides the basename of the default Privoxy logfile in this directory.
         To find the absolute path of the default Privoxy logfile, join these
         two options.  For example, the following options instruct Privoxy to
         log to `/var/log/privoxy/privoxy.log`:

                logdir /var/log/privoxy
                logfile privoxy.log

1. **(Re)start Privoxy.** Under:
   1. `systemd`-based Linux distributions (e.g., Arch, Fedora, Ubuntu), run:

            $ sudo systemctl restart privoxy

   1. OpenRC-based Linux distributions (e.g., Calculate, Sabayon, Gentoo), run:

            $ sudo rc-service privoxy restart

   1. OS X, run:

            $ sudo /Applications/Privoxy/stopPrivoxy.sh
            $ sudo /Applications/Privoxy/startPrivoxy.sh

   1. Windows... **we have no idea.** If _you_ find out how, please update these
      instructions accordingly.
1. **Verify that Privoxy is listening on the expected port.** Under non-Windows
   systems with [netstat](https://en.wikipedia.org/wiki/netstat) installed:
   1. Run:

            $ sudo netstat -atnp | grep privoxy

   1. A line resembling the following should be output:

            tcp        0      0 127.0.0.1:8118          0.0.0.0:*               LISTEN      24526/privoxy

1. **Verify that Privoxy is successfully anonymizing HTTP requests.** Under
   non-Windows systems with [wget](https://www.gnu.org/software/wget) installed:
   1. Show your **unproxied** public IP address (i.e., the globally unique IP
      address of your local machine or network) by running:

            $ wget http://ipinfo.io/ip -qO -

   1. Verify that your unproxied public IP address is printed. For example:

            215.108.10.47

   1. Proxy all subsequent commands through privoxy:

            $ export http_proxy="http://127.0.0.1:8118"

   1. Show your **proxied** public IP address (i.e., the globally unique IP
      address of the Tor exit node to which privoxy forwards all traffic) by
      rerunning the same command:

            $ wget http://ipinfo.io/ip -qO -

   1. Verify that a _different_ IP address is printed. For example:

            58.73.28.81

   1. Cease proxying commands through privoxy:

            $ unset http_proxy

Congratulations, salutations, and good vibrations. Privoxy is up, too!

## Step 3: rTorrent

We will now configure rTorrent to anonymize _all_ torrent traffic through the
previously configured Tor-forwarding privoxy proxy.

1. **Configure rTorrent.** Edit your current `rtorrent.rc` configuration file as
   follows:
   1. **Enable privoxy proxying.**
      1. Remove all existing `http_proxy`, `proxy_address`,
         `network.http.proxy_address.set`, and
         `network.proxy_address.set` options from this file.
      1. Add the following two lines anywhere to this file:

                network.http.proxy_address.set = 127.0.0.1:8118
                network.proxy_address.set = 127.0.0.1:8118

   1. **Disable UDP support.**
      1. Remove all existing `use_udp_trackers`, `dht`, `peer_exchange`,
         `trackers.use_udp.set`, `dht.mode.set`, and `protocol.pex.set` options
         from this file.
      1. Add the following three lines anywhere to this file:

                trackers.use_udp.set = no
                dht.mode.set = disable
                protocol.pex.set = no

1. **(Re)start rTorrent.**
1. **Verify that rTorrent is successfully anonymizing torrent traffic.**
   1. Browse to [ipleak.net](https://ipleak.net), a third-party web service
      reliably detecting IP and DNS leakage from torrent clients.
   1. Click the **Activate** button beneath the _Torrent Address detection_
      heading.
   1. Copy the resulting magnet link (displayed as **this Magnet
      Link**) to the system clipboard. In Firefox, for example, right-click this
      link and choose _Copy Link Location_.
   1. **Keep this page open.** We will return to it shortly. For now, note the
      following text displayed beneath this magnet link:

            No data just now from the above magnet url.

   1. Open rTorrent.
   1. Hit _\<Enter\>_. rTorrent should display an interactive prompt resembling:

            load.normal>

   1. Paste the previously copied magnet link.
   1. Hit _\<Enter\>_ again. A new torrent whose name is a random string of
      alphanumeric characters should now be added.
   1. Hit _\<Ctrl-s\>_ to start this torrent.
   1. Return to your open [ipleak.net](https://ipleak.net) page. If you
      accidentally closed this page, this entire process _must_ be repeated.
   1. Verify that your **proxied** public IP address is now displayed beneath
      this magnet link. As a sanity check, click on this IP address and verify
      that the geolocation of this IP address differs from your own.

Congratulations, salutations, and soul-soothing ministrations. rTorrent is up
and cryptographically secure!

## There's Got To Be Another Way

**There always are.** You just won't like any of them. Viable alternatives
include:

* The [Invisible Internet Project (I2P)](https://geti2p.net), yet another
  open-source anonymity network with similar advantages as Tor (e.g.,
  cross-platform, active development, ongoing research) _without_ the burdensome cultural
  baggage and anti-P2P rhetoric. While detailed instructions for doing so exceed
  the mandates of this guide, it may be pertinent to note that:
  * I2P encourages torrent traffic to be routed through the I2P network.
  * I2P comes bundled with a torrent-specific web client for doing so:
    **I2PSnark.**
  * A variety of I2P [eepsites](http://eepsite.com) (i.e., the I2P equivalent of
    [Tor Hidden
    Services](https://en.wikipedia.org/wiki/List_of_Tor_hidden_services))
    provide PirateBay-like centralized repositories for hosting I2P-only public
    torrents. Common examples include:
    * [Postman](http://tracker2.postman.i2p).
    * [diftracker](http://diftracker.i2p).
  * [Vuze](https://vuze.com), the proprietary torrent client formerly known as
    Azureus and now functionally indistinguishable from [malware-like
    adware](https://en.wikipedia.org/wiki/Azerus#Criticism), provides the [I2P
    Helper](https://wiki.vuze.com/w/I2PHelper_HowTo) plugin. This plugin is perhaps the only
    remaining reason to install Vuze. It bridges clearnet- and
    I2P-hosted torrents, permitting unanonymous clearnet-hosted torrents to be
    anonymized over I2P _and_ anonymous I2P-hosted torrents to be deanonymized
    over the clearnet. No, we have no idea why anyone would want to deanonymize
    themselves either. Nonetheless, the former feature is _awesome incarnate_.
* Subscribing to a non-free anonymization service supporting both HTTP and UDP
  proxying. Common examples include:
  * [Virtual private network
    (VPN)](https://en.wikipedia.org/wiki/Virtual_private_network) providers.
  * [Seedbox](https://en.wikipedia.org/wiki/Seedbox) providers.

Only you can decide your fate.