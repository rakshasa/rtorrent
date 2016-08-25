# Auto-Scraping

**Contents**

 * [Introduction](#introduction)
 * [Problems](#problems)
 * [Realization](#realization)
  * [Config](#config)
  * [Recommendation](#recommendation)

## Introduction

Retrieved information by [scraping](https://wiki.vuze.com/w/Scrape) is only cosmetic using `rTorrent` (since it uses a different philosophy than most other torrent client, it won't affect the operation of it) but good to see these values changing if you use any UI (e.g. webUI) with it or `rTorrent-PS`.

By default, `rTorrent` only sends scrape requests to trackers when a torrent is added for the first time or the client was restarted.

## Problems

- the builtin `t.scrape_time_last` property is lost upon restart (not saved in session)
- [multi-scraping](https://wiki.vuze.com/w/Scrape#Multi-Hash_Requests) isn't implemented


## Realization

Regularly update scrape information for all torrents, even stopped ones.

Let's try to balance the requests to not fire up them at the same time:
- use a custom field `tm_last_scrape` to store the last scrape time per torrent to be able to save into session
- send scrape requests regularly, check for update in every `5 minutes` and distinguish between 2 groups:
  - transferring (uploading/downloading) torrents : update it in every `10 minutes`
  - non-transferring torrents: update it in every `12 hours`

### Config

All you have to do is to copy-paste the following config into your `rTorrent` config. That's it.

```ini
# Regularly update scrape information for all torrents (even stopped ones), it won't affect the operation of rtorrent, but nice to have these values updated.
# This info is only updated when rtorrent starts or a torrent is added by default.
# Try to balance calls to not fire them up at the same time (since multiscraping isn't implemented in libtorrent). Check for update every 5 minutes and distinguish between 2 groups:
#   - transferring (uploading and/or downloading) torrents: update in every 10 minutes
#   - non-transferring torrents: update in every 12 hours
# helper method: sets current time in a custom field (tm_last_scrape) and saves session
method.insert = d.last_scrape.set, simple|private, "d.custom.set=tm_last_scrape,$cat=$system.time=; d.save_full_session="
# helper method: sends the scrape request and sets the tm_last_scrape timestamp and saves session
method.insert = d.last_scrape.send_set, simple, "d.tracker.send_scrape=0;d.last_scrape.set="
# helper method: decides whether the required time interval (with the help of an argument) has passed and if so calls the above method
method.insert = d.last_scrape.check_elapsed, simple|private, "branch={(elapsed.greater,$d.custom=tm_last_scrape,$argument.0=),d.last_scrape.send_set=}"
# helper method: checks for non-existing/empty custom field to be able to test its validity later
method.insert = d.last_scrape.check, simple|private, "branch={d.custom=tm_last_scrape,d.last_scrape.check_elapsed=$argument.0=,d.last_scrape.send_set=}"
# sets custom field (tm_last_scrape) to current time only for torrents just has been added (skips setting time on purpose when rtorrent started)
method.set_key = event.download.inserted_new, last_scrape_i, "d.last_scrape.set="
# check for update every 5 minutes (300 sec) and update scrape info for transferring torrents in every 10 minutes (600-20=580 sec) and for non-transferring ones in every 12 hours (43200-20=43180 sec)
schedule2 = last_scrape_t, 300, 300, "d.multicall2=main,\"branch=\\\"or={d.up.rate=,d.down.rate=}\\\",d.last_scrape.check=580,d.last_scrape.check=43180\""
```

### Recommendation

It's strongly advised to apply the following to your setup:
- [reduce DNS timeout](https://github.com/rakshasa/rtorrent/wiki/Performance-Tuning#dns-timeout)
- [name resolving enhancements](https://github.com/rakshasa/rtorrent/wiki/Performance-Tuning#name-resolving-enhancements)