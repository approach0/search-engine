#!/bin/bash
if [ "$1" == "-h" ]; then
cat << USAGE
Description:
unmount directory specified by mount.log.
$0
USAGE
exit
fi

touch /root/test || exit

mnt=$(cat mount.log)
umount $mnt

rmdir $mnt
