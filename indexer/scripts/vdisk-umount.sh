#!/bin/bash
if [ "$1" == "-h" ]; then
cat << USAGE
Description:
Unmount disk loop-back image.
$0
USAGE
exit
fi

touch /root/test || exit

umount ./tmp
rmdir ./tmp
