#!/bin/bash
### BEGIN INIT INFO
# Provides:          rtorrent_autostart
# Required-Start:    $local_fs $remote_fs $network $syslog $netdaemons
# Required-Stop:     $local_fs $remote_fs
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: rtorrent script using tmux
# Description:       rtorrent script using tmux
### END INIT INFO

#############
###<Notes>###
#############
# This script depends on tmux and is based on 'rtorrentInitScreen.sh' script
# with the following enhancements:
#  - init script 'start' option can be called without breaking anything
#  - init script 'status' option can be used in scripts to determine whether rtorrent is running or not
#  - auto removes damaged/stuck 'rtorrent.lock' file if necessary
#  - saves session (just in case) before stopping with 'rtxmlrp' if it exists
#  - provides examples how to script tmux windows
#
# For the stop function to work, you must set an explicit session directory
# using ABSOLUTE paths (no, ~ is not absolute) in your rtorrent.rc and with
# "sessiondir" option.
# If you typically just start rtorrent with just "rtorrent" on the
# command line, all you need to change is the "user" and "sessiondir" option.
# Attach to the tmux session as your user with
# "tmux -2u new-session -A -s rtorrent". Change "rtorrent" with "tmuxname" option.
# Licensed under the GPLv2 by lostnihilist: lostnihilist _at_ gmail _dot_ com
##############
###</Notes>###
##############

#######################
##Start Configuration##
#######################
# You can specify your configuration in a different file.
# (so that it is saved with upgrades, saved in your home directory,
# or whatever reason you want to)
# by commenting out/deleting the configuration lines and placing them
# in a text file (say /home/user/.rtorrent.init.conf) exactly as you would
# have written them here (you can leave the comments if you desire
# and then uncommenting the following line correcting the path/filename.
# for the one you used. note the space after the ".".
# . /etc/rtorrent.init.conf

# system user to run as
user="username"

# default directory for tmux, needs to be an absolute path
base=$(su -c 'echo $HOME' $user)

# the full path to the filename where you store your rtorrent configuration
config="$base/.rtorrent.rc"

# the full path to the session directory of rtorrent
sessiondir="/mnt/Torrents/.rtorrent/.session"

# options to pass to rtorrent; e.g. don't read config from $HOME but load alternate
#options="-n -O import=$config"
options=""

# name of tmux session
tmuxname="rtorrent"

# name of window in tmux session
tmuxwindowname="rT"
#######################
###END CONFIGURATION###
#######################
PATH=/usr/bin:/usr/local/bin:/usr/local/sbin:/sbin:/bin:/usr/sbin
NAME=rtorrent
DAEMON=$NAME
SCRIPTNAME=/etc/init.d/$NAME
RTXMLRPCBIN="$base/bin/rtxmlrpc"


checkcnfg() {
    if [ -z "$(which $DAEMON)" ] ; then
        echo "Cannot find $DAEMON binary in PATH: $PATH"
        exit 3
    fi
    if ! [ -r "$config" ] ; then
        echo "Cannot find readable config $config. Check that it is there and permissions are appropriate"
        exit 3
    fi
    if ! [ -d "$sessiondir" ] ; then
        echo "Cannot find readable session directory $sessiondir from config $config. Check permissions"
        exit 3
    fi
}


status() {
    if [ -e "${sessiondir}/rtorrent.lock" ] ; then
	pid=`cat ${sessiondir}/rtorrent.lock | awk -F: '{print($2)}' | sed "s/[^0-9]//g"`
	# make sure the pid isn't empty and doesn't belong to another process : this will match lines containing rtorrent, which grep '[r]torrent' does not!
	# if there is no process as the 'pid' suggests then delete the stuck "rtorrent.lock" file (to be able to start rtorrent)
	[[ -n "$pid" ]] && ps aux | grep -sq ${pid}.*[r]torrent && echo -e ${pid} || rm -f "${sessiondir}/rtorrent.lock"
    fi
}


d_start() {
    [ -d "$base" ] && cd "$base"
    # if STDIN is a terminal (we are using interactive mode)
    [ -t 0 ] && stty stop undef && stty start undef
    # start the default 2 tmux window (bash and mc) if there isn't tmux session called "tmuxname" option (rtorrent)
    if ! su -c "tmux ls | grep -sq ${tmuxname}: " $user ; then
	# 1st window (0): split it into 3 panes, display 'date' in the last one
	su -c "tmux -2u new-session -d -s ${tmuxname} -n 'shell1'" $user
	su -c "tmux -2u split-window -v -t ${tmuxname}:0 'bash'" $user
	su -c "tmux -2u split-window -h -t ${tmuxname}:0 'date; bash'" $user
	# 2nd window (1): start 'mc' if it exists
	su -c "tmux -2u new-window -t ${tmuxname}:1 -n 'mc1' 'command which mc && mc ~/; bash'" $user
    fi
    # start rtorrent always in the 3rd tmux window (2) if it's not running and leave shell behind to be able to see reason of a crash
    if [ "$(status)" == "" ]; then
	su -c "tmux -2u list-panes -t ${tmuxname}:2 &>/dev/null && tmux -2u respawn-pane -t ${tmuxname}:2 -k \"${DAEMON} ${options}; bash\" || tmux -2u new-window -t ${tmuxname}:2 -n ${tmuxwindowname} \"${DAEMON} ${options}; bash\"" $user
    fi
}


d_stop() {
    pid=$(status)
    if [ "$pid" != "" ]; then
	# save session before stopping explicitly (just in case) if rtxmlrpc util exists then wait for 5 seconds to be able to complete it
	[ -L "$RTXMLRPCBIN" ] && "$RTXMLRPCBIN" session.save &>/dev/null && sleep 5
	# INT (2, Interrupt from keyboard): normal shutdown
	kill -s INT $pid
    fi
}


checkcnfg


case "$1" in
    start)
        echo -n "Starting $tmuxwindowname: $NAME"
        d_start
        echo "."
        ;;
    stop)
        echo -n "Stopping $tmuxwindowname: $NAME"
        d_stop
        echo "."
        ;;
    restart|force-reload)
        echo -n "Restarting $tmuxwindowname: $NAME"
        d_stop
        sleep 1
        d_start
        echo "."
        ;;
    status)
        status
        ;;
    *)
        echo "Usage: $SCRIPTNAME {start|stop|restart|force-reload|status}" >&2
        exit 1
        ;;
esac

exit 0
