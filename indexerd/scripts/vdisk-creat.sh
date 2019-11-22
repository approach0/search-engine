#!/bin/bash
if [ "$1" == "-h" ]; then
cat << USAGE
Description:
Create disk loop-back image file.
$0 <filesys> <gigbytes>

Examples:
$0 reiserfs 2
$0 btrfs 2
USAGE
exit
fi

[ $# -ne 2 ] && echo 'bad arg.' && exit

filesys="$1"
gigbyte="$2"
mkfs_opts=""

if [ "$filesys" == "reiserfs" ]
then
	mkfs_opts="-ff"
elif [ "$filesys" == "btrfs" ]
then
	mkfs_opts=""
fi

dd if=/dev/zero of=vdisk.img count=${gigbyte}K bs=1024K

mkfs.${filesys} ${mkfs_opts} ./vdisk.img
