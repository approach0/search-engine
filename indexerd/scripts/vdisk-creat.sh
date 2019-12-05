#!/bin/bash
if [ "$1" == "-h" ]; then
cat << USAGE
Description:
Create disk loop-back image file.
$0 <filesys> <count MiB>

Examples:
$0 reiserfs 1K # 1.0 GiB
$0 btrfs 1512  # 1.5 GiB
USAGE
exit
fi

[ $# -ne 2 ] && echo 'bad arg.' && exit

filesys="$1"
count_MB="$2"
mkfs_opts=""

if [ "$filesys" == "reiserfs" ]
then
	mkfs_opts="-ff"
elif [ "$filesys" == "btrfs" ]
then
	mkfs_opts=""
fi

dd if=/dev/zero of=vdisk.img count=${count_MB} bs=1024K

mkfs.${filesys} ${mkfs_opts} ./vdisk.img
