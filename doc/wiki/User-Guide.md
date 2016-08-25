# rTorrent User Guide #

:information_source: | General note about key combinations: ^ means the Ctrl-key. `M-x` means Meta-x (Usually Alt-x or Esc-x).
---: | :----

**Contents**

* [Adding and removing torrents](#adding-and-removing-torrents)
* [Throttling](#throttling)
* [Common Error Messages](#common-error-messages)
* [Navigating](#navigating)
  * [Global Keys](#global-keys)
  * [Main View Keys](#main-view-keys)
  * [Download View Keys](#download-view-keys)
  * [Peer list View Keys](#peer-list-view-keys)
  * [File list View Keys](#file-list-view-keys)
  * [Tracker list View Keys](#tracker-list-view-keys)
* [Main view window.](#main-view-window)
  * [Torrent info line explanation](#torrent-info-line-explanation)
* [Bottom line in GUI shows current info about a lot of things.](#bottom-line-in-gui-shows-current-info-about-a-lot-of-things)
  * [Bottom line explanation](#bottom-line-explanation)
* [The Peers screen](#the-peers-screen)
  * [Explanation](#explanation)
  * [Status](#status)
* [The Tracker list screen](#the-tracker-list-screen)
* [The Chunks seen screen](#the-chunks-seen-screen)
* [Signal handlers](#signal-handlers)


## Adding and removing torrents ##
| Shortcut | Description |
| ------------- | ------------- |
| backspace | Add torrent using an URL or file path. Use tab to view directory content and do auto-complete. Also, wildcards can be used. For example: `~/torrent/*` |
| return | Same as backspace, except the torrent remains inactive. (Use ^s to activate) |
| ^o | Set new download directory for selected torrent. Only works if torrent has not yet been activated. |
| ^s | Start download. Runs hash first unless already done. |
| ^d | Stop an active download, or remove a stopped download. |
| ^k | Stop and close the files of an active download. | 

Note that ^s (and ^q for quit) is often used for terminal control to pause screen output (and ^q to resume). This may interfere with rTorrent. Type `stty -a` to see whether these have been mapped. To remove the mappings, execute the commands
```
stty stop undef
stty start undef
```
before running rTorrent (or reattaching to screen) to leave them undefined. You could also replace `undef` with some other code like ^p. ^d also usually sends end-of-file but ncurses passes this through to rTorrent, `stty eof undef` if you are worried.

To fix this, you may also toggle the flow control in screen with ^a ^f until screen displays "-flow" in the bottom left corner.

## Throttling ##

| Shortcut | Description |
| ------------- | ------------- |
| a/s/d | Increase the upload throttle by 1/5/50 KB. | 
| z/x/c | Decrease the upload throttle by 1/5/50 KB. | 
| A/S/D | Increase the download throttle by 1/5/50 KB. | 
| Z/X/C | Decrease the download throttle by 1/5/50 KB. | 

Note that all throttling is applied globally and not per torrent.

## Common Error Messages ##
**Could not parse bencoded data**<br>
This message is caused by bad communication with the tracker, often caused by invalid client authentication (passkey, IP address, etc.)

**Could not create download, the input is not a valid torrent**<br>
This message is caused by a corrupted or otherwise non-valid .torrent file. You should redownload the .torrent file or possibly find a new source for it.

**Download registered as completed, but hash check returned unfinished chunks.**<br>
This message is caused by deleting/modifying the downloaded file after a torrent is marked as complete. You could set the 'create/resize queued' flags on all files of the torrent by pressing ^e in main view.

## Navigating ##

### Global Keys ###

| Shortcut | Description |
| ------------- | ------------- |
| ^q | Initiate shutdown, press again to force the shutdown and skip sending the stop signal to trackers. | 
| up/down | Select item. | 
| left | Go back to the previous screen. | 

### Main View Keys ###

| Shortcut | Description |
| ------------- | ------------- |
| right |  Switch to Download View. | 
| ^x | Call commands or change settings. | 
| ^e | Set  the 'create/resize queued' flags on all files in a torrent. This is necessary if the underlying files in a torrent have been deleted or truncated, and thus rtorrent must recreate them. |
| ^r | Initiate hash check of torrent. Without starting to download/upload. | 
| +/- | Change priority of torrent. | 
| l | View log. Exit by pressing the space-bar. | 
| I | Toggle whether torrent ignores or heeds commands. | 
| U | Clear tied to file association for torrent. | 
| 1 | Show all downloads | 
| 2 | Show all downloads, ordered by name | 
| 3 | Show started downloads | 
| 4 | Show stopped downloads | 
| 5 | Show complete downloads | 
| 6 | Show incomplete downloads | 
| 7 | Show hashing downloads | 
| 8 | Show seeding downloads | 
| 9 | Show leeching downloads | 
| 0 | Show active downloads | 

### Download View Keys ###

| Shortcut | Description |
| ------------- | ------------- |
| right | Switch to selected view | 
| left | Switch to view selection or back to main view | 
| 1/2 | Adjust max uploads. | 
| 3/4 | Adjust min peers. | 
| 5/6 | Adjust max peers. | 
| p | Display peer list | 
| o | Display torrent info | 
| i | Display file list | 
| u | Display tracker list | 
| t/T | Initiate tracker request. Use capital "T" to force the request, ignoring the "min interval" set by the tracker. | 

### Peer list View Keys ###

| Shortcut | Description |
| ------------- | ------------- |
| left | Switch to view selection | 
| right | Show peer details | 
| * | Snub peer (stop uploading to this peer) | 
| k | Kick peer (disconnect from peer) | 
| B | Ban peer (No unbanning is possible. Requires rTorrent 0.8.4+) | 

### File list View Keys ###

| Shortcut | Description |
| ------------- | ------------- |
| left | Switch to view selection | 
| right | Show file details | 
| space | Change the file priority; applies recursively when done on a directory | 
| * | Change the priority of all files | 
| / | Collapse directories. While collapsed, press right to expand the selected directory. | 

Priority options are _blank_ (standard priority), `hig` (high priority) and `off` (not to be downloaded).

### Tracker list View Keys ###

| Shortcut | Description |
| ------------- | ------------- |
| left | Switch to view selection | 
| * | Enable/disable tracker | 
| space | Rotate trackers in a group | 

## Main view window. ##

The main view window lists all loaded torrents with respective status info. Example of a single torrent:

```
*  ubuntu-5.10-install-i386.iso
* Torrent:  161.6 /  617.2 MB Rate:   1.1 /  41.3 KB Uploaded:     5.1 MB [24%]  0d  3:09 [TI R: 0.03]
*
```

###Torrent info line explanation###

**Torrent:  161.6 /  617.2 MB**  
     Amount of downloaded data / total size of torrent.

**Rate:   1.1 /  41.3 KB**  
     Upload / download speed rates.

**Uploaded:     5.1 MB**  
     Amount of uploaded data.

**[24%]**  
     Download ratio. 161.6 MB downloaded makes 24% of the total 617.2 MB. Well, roughly apparently, presumably since some of the downloaded data may have been bad and discarded.

**0d  3:09**  
     Estimated completion time. This torrent is expected to be completely downloaded in 0 days, 3 hours and 9 minutes.

**T**  
     Present if torrent is tied to a file, blank otherwise.

**I**  
     Present if torrent is ignoring commands (such as stop_on_ratio). Toggle with 'I'.

**R: 0.03**  
     Share ratio. 5.1 MB uploaded makes 3% of the 161.6 MB already downloaded.


Note that the estimated completion time does not take into account whether you turned off the download of individual files in the torrent. It just relies on the current download speed and total size of the torrent.

## Bottom line in GUI shows current info about a lot of things. ##

Here are a few outlined.

Example of the status bar:
```
Throttle U/D: 200/off  Rate: 141.6 /   0.0 KiB  Listen:<default>:xxxxx  Bind: xxx.xxx.xxx.xxx  [U 3/14] [D 15/0] [H 1/32] [S 6/40/768] [F 4/128]
```

###Bottom line explanation###

**Throttle U/D: 200/off**  
These are the current speed settings.  
U = upload = outgoing transfer  
D = download = incoming transfer  
In this case the total upload speed is limited to 200 KB/s.  
With the D setting to "off" there is no limit on download speed.

See Throttling section for information on how to change the settings.

**Rate: 141.6 /   0.0 KiB**  
     These numbers show the current upload/download speeds.

**Listen:<default>:xxxxx**  
     This is the ip reported to the tracker.  
     It should be the same as the ip from where you are browsing the tracker.

**Bind: xxx.xxx.xxx.xxx**  
     This is the ip on which libtorrent/rtorrent is running.  
     The external ip if you're behind a NAT.

**[D 15/0]**  
     Current number of download slots in use and the maximum (0 if unlimited).

**[H 1/32]**  
     Current number of active HTTP requests (for tracker announces and downloads of .torrent files), and the maximum.

**[U 3/14]**  
     Current number of upload slots in use, and the maximum, which depends on the global upload rate limit.

**[S 6/40/768]**  
     The three numbers represent handshakes/open sockets/max open sockets.

**[F 4/128]**  
     The two numbers represent open files/max open files. The library dynamically closes the least used files as needed.

## The Peers screen ##

Some parts of this screen is fairly cryptic, here's an explanation of the fields.

Header:
```
IP              UP     DOWN   PEER   C/RE/LO  QS    DONE  REQ   SNUB
```
Example entry:
```
1.1.1.1         1.1    0.0    20.5   r/ci/un  3/0   32
```


###Explanation###

**IP**  
     The IP address of the peer. (Which, of course, has nothing to do with DNS)

**UP**  
     The rate at which your client is uploading to the peer (KiB/sec).

**DOWN**  
     The rate at which your client is downloading from the peer (KiB/sec).

**PEER**  
     The download rate (KiB/sec) of this peer for this torrent (reported to your client by the peer client).

**C/RE/LO**  
C  = Connection type, can be r, l, R or L.
&nbsp;&nbsp;&nbsp;&nbsp;r = Started remotely, the peer initiated the connection to your client.  
&nbsp;&nbsp;&nbsp;&nbsp;l = Started locally, your client initiated the connection to the peer.  
&nbsp;&nbsp;&nbsp;&nbsp;R = Started remotely, using encrypted data transfer.  
&nbsp;&nbsp;&nbsp;&nbsp;L = Started locally, using encrypted data transfer.

RE = Remote client information, has two parts; the first is u or c and the second is i or n.  
&nbsp;&nbsp;&nbsp;&nbsp;c = This peer has choked your client (which means it is not going to send you any pieces for now).  
&nbsp;&nbsp;&nbsp;&nbsp;u = Your client is not choked by this peer (unchoked).  
&nbsp;&nbsp;&nbsp;&nbsp;i = This peer is interested in downloading from your client.  
&nbsp;&nbsp;&nbsp;&nbsp;n = This peer is not interested in downloading from your client. 

LO = Local client information, has two parts; the first is u or c and the second is i or n.  
&nbsp;&nbsp;&nbsp;&nbsp;c = Your client has choked this peer (which means it is not going to send this peer any pieces for now).  
&nbsp;&nbsp;&nbsp;&nbsp;u = Your client is not choking the peer (unchoked).  
&nbsp;&nbsp;&nbsp;&nbsp;i = Your client is interested in downloading from this peer.  
&nbsp;&nbsp;&nbsp;&nbsp;n = Your client is not interested in downloading from this peer.

**QS**  
Queues Outoing/Incoming pieces.  
The first is the number of pieces that are queued to be sent to the peer.  
The second is the number of pieces your client has requested and is waiting for the peer to send.

**DONE**  
What percent this peer has of the file.

**REQ**  
The number of the piece that is currently on the top of the request queue.

**SNUB**  
This field has a * in it when your client is snubbing this peer. When a peer agrees to send you a piece you have requested and then does not send that piece within a certain time your client will snub this peer. It is basically a way for the client to flag that this peer is unreliable and it is better to request pieces from other clients.

###Status###

A status line in this screen lists information such as:
```
Peers: 99(1002) Min/Max: 40/100 Uploads: 15 U/I/C/A: 3/71/5/3 Failed: 0
```
**Peers**  
     The amount of peers you are connected and (not connected) to.

**Min/Max**  
     The minimum and maximum amount of peers to try to keep connected. Can be toyed with keys 3, 4, 5 and 6.

**Uploads**  
     Maximum number of peers to upload to at the same time (given the global upload slots are available). Can be toyed with keys 1 and 2.

**U/I/C/A**  
U = Number of corrently unchoked peers  
I = Currently interested peers  
C = Complete peers  
A = Peers accounted  

**Failed**  
Number of failed chunks.

## The Tracker list screen ##

<pre><code> Trackers: [Key: zxcvbnma] [req          ]
0: http://tracker.publicbt.com:80/announce
    Id:  Counters: 0f / 1s (0)  on S/L/D: 114/42/0 (0/50)
1: udp://tracker.openbittorrent.com:80/announce
    Id: 2527549 Counters: 0f / 705s (18)  on S/L/D: 30/1/285 (2/15)</code></pre>

Displays the available trackers for a torrent with their announce URL and different properties.

**Possible values in the header**
req = requesting  
prom = promiscuous mode  
fail = failure mode  

**Properties of a tracker**
Properties from left to right, using the second (1) tracker as an example:
Id: 2527549 = id (can be empty)  
Counters: 0f / 705s (18) = failed / successful (scrape) counters  
on = status , [on|off|err] , on -> usable , off -> disabled , err -> error  
S/L/D: 30/1/285 = number of seeders/leechers/downloads reported by the tracker during a scrape request  
(2/15) = number of new peers received last request / total number of peers received  

## The Chunks seen screen ##

<pre><code> Chunks seen: [C/A/D 25/29/6.99]     X downloaded  <strong>X</strong> missing  <strong><u>X</u></strong> queued  <span style="background-color: black; color: white">X</span> downloading
    0 EFD<strong>FEF</strong>FDFE <strong>CEFFFFF</strong>FFF EFCFC<strong>FDDBC 9DFDBDDECF BDDB</strong>CEDDFD DEFEEDECFC
   60 DBDFEE9<span style="background-color: black; color: white">B</span><strong><u>EC</u> <u>FCCBECC</u>FEB FAFDDFCAAD CABCDECDFC 9BFDCDEFAA ADCEFFBBDC
  </strong>120 <strong>FAFFCCBEAC EDFFEDDF9B EDD9DEBDDD C9BCEAFF9C F8FDCDDBFB CFCECFCEDD
  </strong>180 <strong>FFFADCCFAE FB9F</strong>BFC<strong>CAC BE</strong>ABBB9FD<strong>D ABCECEDFAB DCF9BDDFBD BBEFAE7EED
  </strong>240<strong> DDCEAFBCF7 EEEFBF9AAD 9FDBCDDEBB DDF79C8FCE EFF9FDFE9A EF9DCCEEBA</strong>
</code></pre>



C/A/D stands for Complete/Accounted/Distributed copies. Only those peers whose bitfields are accounted for are included above; they do not include seeders, peers with empty bitfields, nor normal peers if there are too many.

Then, you have a view of the chunks of the torrent. The chunks are shown in groups of 10. Each new line begins with the number of the first chunk on that line. 

Each chunk is shown by a hexadecimal number (0123456789ABCDEF) which is equal to its rarity among the non-seeders: 

 * a chunk shown as 0 is not available from non-seeders in the swarm (but all seeders have it of course)
 * a chunk shown as 6 is available from 6 non-seeders
 * and a chunk shown as F is available from 15 or more peers. 

If the chunk is in **bold** it means you do not have that chunk yet, if it is <u>underlined</u> then it is in the transfer list (i.e. queued to be downloaded), and if the chunk is in reverse colors, it is currently being downloaded. Otherwise, a normal font indicates that the chunk has already been downloaded and verified as correct by rTorrent.


## Signal handlers ##

| Signal | Description |
| ------------- | ------------- |
|  SIGINT  |  Normal shutdown with 5 seconds to send the stopped request to trackers.  | 
|  SIGTERM  |  Shut down immediately.  | 
|  SIGWINCH  |  Resize and redraw.  | 