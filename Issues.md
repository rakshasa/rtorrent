# Known Issues

## Using the 'ntfs' File System

`rtorrent` has issues with **ntfs** partitions.
It's reported that `rtorrent` freezes when downloading on partition formatted with ntfs filesystem.[1]

Also, if the files are bigger than 4GB, data gets corrupted.[2]


***

1. http://askubuntu.com/questions/399775/constant-freezes-when-downloading-torrent-to-ntfs-partition
2. https://bbs.archlinux.org/viewtopic.php?id=192811
3. https://github.com/rakshasa/rtorrent/issues/194
4. https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=743734
5. Sorting/filtering 'started' and 'stopped' views causes announces to fire with v0.9.6: https://github.com/rakshasa/rtorrent/issues/449