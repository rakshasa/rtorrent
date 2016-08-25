# The `system.*` Command Group


## system.env=NAME (since 0.9.7)

Query the value of an environment variable, returns an empty string if `NAME` is not defined. Example:

```ini
session.path.set=(cat,(system.env,RTORRENT_HOME),"/.session")
```