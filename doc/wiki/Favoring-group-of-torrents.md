# Favoring one group of torrents over the rest of them

This is one of the most frequently asked questions regarding to the usage of `rTorrent`. It turned out that its realization is pretty simple: let's adjust upload rate dynamically for the rest and other attributes of them.

This feature is so powerful that it will change the way you used to work with `rTorrent`.

**Contents**

 * [Scenario](#scenario)
 * [Theory](#theory)
 * [Realization](#realization)
  * [Sample config](#sample-config)
  * [Helper script](#helper-script)
  * [How it works](#how-it-works)
    * [Getting new uprate limit for slowup throttle](#getting-new-uprate-limit-for-slowup-throttle)
    * [Getting new global downrate limit](#getting-new-global-downrate-limit)

## Scenario

There are sites where seeding data back is pretty hard while it's easy for others. So we want to distinguish between 2 groups, a special group and the rest of them:
- special group: we want to favor this group in any possible way
- the rest: it's not so important to provide the best circumstances for them, but we still want to seed them as long as we can

Applying this to downloading is easy (all we have to set is `d.priority` accordingly and it will take care about everything) but not to seeding.


## Theory

Let's take a look what we need for this:
- define 1 `throttle.up` group named `slowup` and set an inital value for it that will contain all the unimportant torrents (torrents that are assigned to this group)
- torrents that don't belong to the `slowup` throttle belong to the special group
- set `d.priority` to high for the special group that will take care of the downloading part
- raise the default `min_peers`, `max_peers` and `max_uploads` values for the special group (note: these settings aren't saved along the session hence we have to modify them on the fly, if necessary)
- setup 2 watch directories to automate this assignment between torrents and groups
- adjust the upload rate of `slowup` throttle on the fly (with the help of an external script) depending on the current upload rate of the special group and the current global upload rate  

Since all we want to do is seeding (unfortunately we have to download before it :) ), we can also deal with the common problem of asynchronous connections (e.g. ADSL) where upload speed can be slowed down if download speed is close to maximum bandwidth:
- adjusts the global download rate dynamically to always leave enough bandwidth to the upload rate of 1st main (special) group


## Realization

### Sample config

Let's see the related settings at once first. These settings are used with 74/20 Mbps connection and with `rTorrent v0.9.6`.

```ini
# Global upload and download rate in KiB, `0` for unlimited (`download_rate`, `upload_rate`)
throttle.global_down.max_rate.set_kb = 8700
throttle.global_up.max_rate.set_kb   = 2200

# Maximum number of simultaneous downloads and uploads slots (global slots!) (`max_downloads_global`, `max_uploads_global`)
throttle.max_downloads.global.set = 300
throttle.max_uploads.global.set   = 300

# Maximum and minimum number of peers to connect to per torrent while downloading (`min_peers`, `max_peers`) Default: `100` and `200` respectively
throttle.min_peers.normal.set = 99
throttle.max_peers.normal.set = 100

# Same as above but for seeding completed torrents (seeds per torrent), `-1` for same as downloading (`min_peers_seed`, `max_peers_seed`) Default: `-1` for both
throttle.min_peers.seed.set = -1
throttle.max_peers.seed.set = -1

# Maximum number of simultaneous downloads and uploads slots per torrent (`max_uploads`) Default: `50` for both
throttle.max_downloads.set = 50
throttle.max_uploads.set = 50

# Throttles rates for (groups of) downloads or IP ranges. (throttle_up) These throttles borrow bandwidth from
#  the global throttle and thus are limited by it too.
# You can assign throttles to a stopped (!) download with Ctrl-T. The `NULL` throttle is a special unlimited
#  throttle that bypasses the global throttle.
# Limits the upload rate to 600 kb/s for the `slowup` throttle group. We also use this property to distinguish
#  `special` and `rest` group. This value will be auto-adjusted by a helper script in Favoring section.
throttle.up = slowup,600

# initial `slowup` group values for the previous similar 3 settings that will be overridden by per torrent settings
#  Favouring section. `cfg.slowup.d.peers_min` ~ `throttle.min_peers.normal` ,
#  `cfg.slowup.d.peers_max` ~ `throttle.max_peers.normal` , `cfg.slowup.d.uploads_max` ~ `throttle.max_uploads`
method.insert = cfg.slowup.d.peers_min,   value|private,  29
method.insert = cfg.slowup.d.peers_max,   value|private,  50
method.insert = cfg.slowup.d.uploads_max, value|private,  15


# Watch directories for new torrents (meta files). Also specifying whether they belong to special group by setting:
#  - high priority for the special group 
#  - `slowup` throttle for `rest` group
schedule2 = special_watch_dir,  5,  10, "load.start=\"./watch/start_special/*.torrent\", d.priority.set=3"
schedule2 = rest_watch_dir,     6,  10, "load.start=\"./watch/start_rest/*.torrent\",    d.throttle_name.set=slowup"


# Set `pyro.bin_dir` to users `bin` directory. Make sure you end it with a "/".
#  If this is left empty, then the shell's path is searched.
method.insert = pyro.bin_dir,     string|const|private, (cat,"~/bin/")


##### begin: Favoring special group of torrents over the rest #####

# helper method: Modifies the `peers_min`, `peers_max`, `max_uploads` values of a torrent for
#  both downloading and uploading
method.insert = d.modify_slots, simple|private, "d.peers_min.set=(argument.0); d.peers_max.set=(argument.1); d.uploads_max.set=(argument.2)"
# Modifies the above properties for a torrent based on which group it belongs to
method.insert = d.modify_slots_both, simple, "branch=((not,(d.throttle_name))),((d.modify_slots,(throttle.min_peers.normal),(throttle.max_peers.normal),(throttle.max_uploads))),((d.modify_slots,(cfg.slowup.d.peers_min),(cfg.slowup.d.peers_max),(cfg.slowup.d.uploads_max)))"
# Modify both group values when a torrent is resumed (even after hash-checking or after `rTorrent` is restarted)
method.set_key = event.download.resumed, modify_slots_resumed_both, "d.modify_slots_both="

# helper method: gets one of the below info with the help of `getLimits.sh` script 
#  note: variables are inside the script, you have to edit those values there!
method.insert = get_limit, simple|private, "execute.capture=\"$cat=$pyro.bin_dir=,getLimits.sh\",$argument.0=,$argument.1=,$argument.2=,$argument.3="
# Dynamically adjusts the 2nd group (`slowup` throttle) uprate (upload speed) to always leave enough bandwidth
#  to the 1st main group. (More info in `getLimits.sh` script)
schedule2     = adjust_throttle_slowup, 14, 20, "throttle.up=slowup,\"$get_limit=$cat=up,$convert.kb=$throttle.global_up.rate=,$convert.kb=$throttle.up.rate=slowup\""
# Dynamically adjusts the global download rate to always leave enough bandwidth to the 1st main group upload rate.
#  It's good for async connection (e.g. ADSL) where upload speed can be slowed down if download speed is
#  close to max. Comment it out if you don't need it.
schedule2     = adjust_throttle_global_down_max_rate, 54, 60, "throttle.global_down.max_rate.set_kb=\"$get_limit=$cat=down,$convert.kb=$throttle.global_up.rate=,$convert.kb=$throttle.up.rate=slowup\""
# Helper method to display the current rate information: `^x` + `i=`.
#  It displays: MainUpRate: 1440 , ThrottleUpRate: 92 , ThrottleLimit: 100
method.insert = i, simple, "print=\"$get_limit=$cat=info,$convert.kb=$throttle.global_up.rate=,$convert.kb=$throttle.up.rate=slowup,$convert.kb=$throttle.up.max=slowup\""

##### end: Favoring special group of torrents over the rest #####
```


### Helper script

Since we can't do basic arithmetic operations in `rTorrent` config files, we need the [`getLimits.sh`](https://github.com/chros73/rtorrent-ps_setup/blob/master/ubuntu-14.04/home/chros73/bin/getLimits.sh) external script that should be placed under `~/bin` directory (it's only linked here because it's too long with comments).

It can do 3 things:
- gets new uprate limit for `slowup` throttle (based upon the configured variables and the given parameters)
 - specify the top of the cap (`sluplimit`: highest allowable value in KiB, e.g. `1700`) for `slowup` throttle
 - specify the bottom of the cap (`sldownlimit`: lowest allowable value in KiB, e.g. `100`) for `slowup` throttle
- gets new global downrate limit (based upon the configured variables and the given parameters)
 - specify the top of the cap (`gldownlimitmax` in KiB, e.g. `9999`) for global downrate
 - specify the bottom of the cap (`gldownlimitmin` in KiB, e.g. `7500`) for global downrate
 - specify the global uprate limit (`alluplimit` in KiB, e.g. `1600`) that above this value it should lower global downrate
 - specify the main (special) uprate limit (`mainuplimit` in KiB, e.g. `1200`) that above this value it should lower global downrate
- gets info about current speed and limits in the form of: `MainUpRate: 1440 , ThrottleUpRate: 92 , ThrottleLimit: 100`
 - note: this functionality is deprecated, use [this patch for rTorrent / rTorrent-PS](https://github.com/rakshasa/rtorrent/pull/447) or [rTorrent-PS-CH](https://github.com/chros73/rtorrent-ps/blob/master/README.rst#fork-notes) to be able to get realtime info about these

You have to edit the variables at the beginning of the script according to your needs, it probably also needs some experimenting which values works best for your needs. There's one more advantage of including variables in the script itself that you can modify them while `rTorrent` is running and the new values will be used next time when the script is called.


### How it works

Once you "installed" both the sample config and the helper script then it works already:
- once you put a torrent into a watch directory all the necessary attributes are assigned to it
- `adjust_throttle_slowup` runs in every 20 seconds
- `adjust_throttle_global_down_max_rate` runs in every 60 seconds

#### Getting new uprate limit for slowup throttle

It doesn't use a complicated algorithm to always give back a reliable result, `sluplimit - mainup`, but it works pretty efficiently:
- checks the current uprate of throttle and uprate of main (special) group (with the help of the global uprate) then it raises or lowers the throttle limit according to the uprate of the main group
- you should leave a considerable amount of gap ~`20%` (~`500 KiB`, in this case) between the top of the cap (`sluplimit` , e.g. `1700`) and the max global upload rate (upload_rate : the global upload limit , e.g. `2200`) to be able to work efficiently: to leave bandwidth to the main (special) group between checks (within that 20 seconds interval)

#### Getting new global downrate limit

This one even less sophisticated then the above one, it only sets one of the 2 defined `gldownlimitmax`, `gldownlimitmin` values for global downrate:
- whether global upload rate is bigger than the specified global uprate limit (`alluplimit`) and main (special) upload rate is bigger than the specified main uprate limit (`mainuplimit`)

Remember, if you don't need this feature then comment out `adjust_throttle_global_down_max_rate` scheduling in `rTorrent` config.
