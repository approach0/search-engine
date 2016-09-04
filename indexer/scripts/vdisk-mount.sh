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

if [ "$filesys" == "reiserfs" ]
then
	mount_opts=""
elif [ "$filesys" == "btrfs" ]
then
	mount_opts="-o compress=lzo,ssd"
fi

# mount
mkdir -p ./tmp
set -x
mount -t ${filesys} ${mount_opts} ./vdisk.img ./tmp
set +x
chmod 777 ./tmp
