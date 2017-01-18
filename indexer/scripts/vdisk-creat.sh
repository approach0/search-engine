#!/bin/bash
if [ "$1" == "-h" ]; then
cat << USAGE
Description:
Create disk loop-back image file.
$0 <filesys>

Examples:
$0 reiserfs
$0 btrfs
USAGE
exit
fi

[ $# -ne 1 ] && echo 'bad arg.' && exit

filesys="$1"
mkfs_opts=""

if [ "$filesys" == "reiserfs" ]
then
	mkfs_opts="-ff"
elif [ "$filesys" == "btrfs" ]
then
	mkfs_opts=""
fi

dd if=/dev/zero of=vdisk.img count=5K bs=1024K # 4GB

mkfs.${filesys} ${mkfs_opts} ./vdisk.img
