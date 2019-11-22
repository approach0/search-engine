#!/bin/bash
if [ "$1" == "-h" ]; then
cat << USAGE
Description:
Mount to disk loop-back image file.
$0 <filesys> <mount image file>

Examples:
$0 reiserfs vdisk.img
$0 btrfs vdisk.img
USAGE
exit
fi

[ $# -ne 2 ] && echo 'bad arg.' && exit
touch /root/test || exit

filesys="$1"
mnt_img="$2"
mnt_dir="mnt-$(basename ${mnt_img})"

if [ ! -e "$mnt_img" ]; then
	echo "image $mnt_img not existing, abort."
	exit
fi

if [ "$filesys" == "reiserfs" ]; then
	mount_opts=""
elif [ "$filesys" == "btrfs" ]; then
	mount_opts="-o compress=lzo,ssd"
fi

# mount
mkdir -p "${mnt_dir}"
set -x
mount -t ${filesys} ${mount_opts} ${mnt_img} ${mnt_dir}
set +x
chmod 777 "${mnt_dir}"
