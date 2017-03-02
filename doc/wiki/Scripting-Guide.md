# rTorrent Scripting Explained

:warning: | This guide is still very incomplete — the best way to remedy that is to contribute what you know.
---: | :---

You can use the quite powerful GitHub search to find information on commands, e.g. their old vs. new syntax variants, what they actually do (i.e. “read the source”), and internal uses in predefined methods, handlers, and schedules. 
Consider the [view.add](https://github.com/rakshasa/rtorrent/search?utf8=%E2%9C%93&q=%22view.add%22) example.

 * [Sending commands with XMLRPC2SCGI](https://github.com/rakshasa/rtorrent/wiki/RPC-Utility-XMLRPC2SCGI)
 * Auto-generated list of [options](https://github.com/rakshasa/rtorrent/wiki/RPC-Option-Strings) used with some commands.


## Commands Reference

Command | Short Description
---: | :---
[execute](https://github.com/rakshasa/rtorrent/wiki/COMMAND-Execute) | Call operating system commands, possibly catching their output for use within rTorrent.
[schedule](https://github.com/rakshasa/rtorrent/wiki/COMMAND-Scheduling) | Repeatedly execute commands, either in a given frequency, or at certain times.
[system.*](https://github.com/rakshasa/rtorrent/wiki/COMMAND-System) | Commands related to the operating system and the XMLRPC API.
[ui.*](https://github.com/rakshasa/rtorrent/wiki/COMMAND-UserInterface) | These commands control aspects of the ‘curses’ UI.


## Variable types

This is a summary about the possible variable types in [command_dynamic.cc](https://github.com/rakshasa/rtorrent/blob/master/src/command_dynamic.cc) (applies to v0.9.6).

Available types:

 * multi (with subtypes: static, private, const, rlookup)
   * TODO: what is it
 * simple (with subtypes: static, private, const)
   * TODO: why is it "simple"
 * value, bool, string, list (with subtypes: static, private, const)
   * Standard types, "value" is an integer.