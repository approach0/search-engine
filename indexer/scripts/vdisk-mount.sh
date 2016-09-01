#!/bin/bash
if [ "$1" == "-h" ]; then
cat << USAGE
Description:
Mount to disk loop-back image file.
$0 <filesys>

Examples:
$0 reiserfs
$0 btrfs
USAGE
exit
fi

[ $# -ne 1 ] && echo 'bad arg.' && exit
touch /root/test || exit

filesys="$1"

# mount
mkdir -p ./tmp
chmod 777 ./tmp
mount -t ${filesys} ./vdisk.img ./tmp
