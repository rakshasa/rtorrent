#!/bin/bash
#############
###<Notes>###
#############
# This script depends on screen.
# For the stop function to work, you must set an
# explicit session directory using absolute paths (no, ~ is not absolute) in your rtorrent.rc.
# If you typically just start rtorrent with just "rtorrent" on the
# command line, all you need to change is the "user" option.
# Attach to the screen session as your user with 
# "screen -dr rtorrent". Change "rtorrent" with srnname option.
# Licensed under the GPLv2 by lostnihilist: lostnihilist _at_ gmail _dot_ com
##############
###</Notes>###
##############

#######################
##Start Configuration##
#######################
# You can specify your configuration in a different file 
# (so that it is saved with upgrades, saved in your home directory,
# or whatever reason you want to)
# by commenting out/deleting the configuration lines and placing them
# in a text file (say /home/user/.rtorrent.init.conf) exactly as you would
# have written them here (you can leave the comments if you desire
# and then uncommenting the following line correcting the path/filename 
# for the one you used. note the space after the ".".
# . /etc/rtorrent.init.conf


#Do not put a space on either side of the equal signs e.g.
# user = user 
# will not work
# system user to run as (can only use one)
user="user"

# system user to run as # not implemented, see d_start for beginning implementation
# group=$(id -ng "$user")

# the full path to the filename where you store your rtorrent configuration
# must keep parentheses around the entire statement, quotations around each config file
config=("$(su -c 'echo $HOME' $user)/.rtorrent.rc")
# Examples:
# config=("/home/user/.rtorrent.rc")
# config=("/home/user/.rtorrent.rc" "/mnt/some/drive/.rtorrent2.rc")
# config=("/home/user/.rtorrent.rc"
# "/mnt/some/drive/.rtorrent2.rc"
# "/mnt/another/drive/.rtorrent3.rc")

# set of options to run with each instance, separated by a new line
# must keep parentheses around the entire statement
#if no special options, specify with: ""
options=("")
# Examples:
# starts one instance, sourcing both .rtorrent.rc and .rtorrent2.rc
# options=("-o import=~/.rtorrent2.rc")
# starts two instances, ignoring .rtorrent.rc for both, and using
# .rtorrent2.rc for the first, and .rtorrent3.rc for the second
# we do not check for valid options
# options=("-n -o import=~/.rtorrent2.rc" "-n -o import=~/rtorrent3.rc")

# default directory for screen, needs to be an absolute path
base=$(su -c 'echo $HOME' $user)

# name of screen session
srnname="rtorrent"

# file to log to (makes for easier debugging if something goes wrong)
logfile="/var/log/rtorrentInit.log"
#######################
###END CONFIGURATION###
#######################

PATH=/usr/bin:/usr/local/bin:/usr/local/sbin:/sbin:/bin:/usr/sbin
DESC="rtorrent"
NAME=rtorrent
DAEMON=$NAME
SCRIPTNAME=/etc/init.d/$NAME

checkcnfg() {
  exists=0
  for i in `echo "$PATH" | tr ':' '\n'` ; do
    if [ -f $i/$NAME ] ; then
      exists=1
      break
    fi
  done
  if [ $exists -eq 0 ] ; then
    echo "cannot find $NAME binary in PATH: $PATH" | tee -a "$logfile" >&2
    exit 3
  fi
  for (( i=0 ; i < ${#config[@]} ;  i++ )) ; do
    if ! [ -r "${config[i]}" ] ; then
        echo "cannot find readable config ${config[i]}. check that it is there and permissions are appropriate"  | tee -a "$logfile" >&2
        exit 3
    fi
    session=$(getsession "${config[i]}")
    if ! [ -d "${session}" ] ; then
        echo "cannot find readable session directory ${session} from config ${config[i]}. check permissions" | tee -a "$logfile" >&2
        exit 3
    fi
  done
}

d_start() {
  [ -d "${base}" ] && cd "${base}"
  stty stop undef && stty start undef
  su -c "screen -S "${srnname}" -X screen rtorrent ${options} 2>&1 1>/dev/null" ${user} | tee -a "$logfile" >&2
  # this works for the screen command, but starting rtorrent below adopts screen session gid
  # even if it is not the screen session we started (e.g. running under an undesirable gid
  #su -c "screen -ls | grep -sq "\.${srnname}[[:space:]]" " ${user} || su -c "sg \"$group\" -c \"screen -fn -dm -S ${srnname} 2>&1 1>/dev/null\"" ${user} | tee -a "$logfile" >&2
  for (( i=0 ; i < ${#options[@]} ; i++ )) ;  do
    sleep 3
    su -c "screen -S "${srnname}" -X screen rtorrent ${options[i]} 2>&1 1>/dev/null" ${user} | tee -a "$logfile" >&2
  done
}

d_stop() {
  for (( i=0 ; i < ${#config[@]} ; i++ )) ; do
    session=$(getsession "${config[i]}")
    if ! [ -s ${session}/rtorrent.lock ] ; then
        return
    fi
    pid=$(cat ${session}/rtorrent.lock | awk -F: '{print($2)}' | sed "s/[^0-9]//g")
    # make sure the pid doesn't belong to another process
    if ps -A | grep -sq ${pid}.*rtorrent ; then
        kill -s INT ${pid}
    fi
  done
}

getsession() { 
    session=$(cat "$1" | grep "^[[:space:]]*session.path.set[[:space:]]*=" | sed "s/^[[:space:]]*session.path.set[[:space:]]*=[[:space:]]*//" )
    #session=${session/#~/`getent passwd ${user}|cut -d: -f6`}
    echo $session
}

checkcnfg

case "$1" in
  start)
    echo -n "Starting $DESC: $NAME"
    d_start
    echo "."
    ;;
  stop)
    echo -n "Stopping $DESC: $NAME"
    d_stop
    echo "."
    ;;
  restart|force-reload)
    echo -n "Restarting $DESC: $NAME"
    d_stop
    sleep 1
    d_start
    echo "."
    ;;
  *)
    echo "Usage: $SCRIPTNAME {start|stop|restart|force-reload}" >&2
    exit 1
    ;;
esac

exit 0
