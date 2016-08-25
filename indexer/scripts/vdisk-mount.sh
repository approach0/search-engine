#!/bin/bash
if [ "$1" == "-h" ]; then
cat << USAGE
Description:
mount disk file partitioned in reiserfs.
$0 <usr> <mnt dir>

Examples:
sudo $0 `whoami` ./tmp
USAGE
exit
fi

[ $# -ne 2 ] && echo 'bad arg.' && exit
touch /root/test || exit

me="$1"
mnt="$2"

echo "mount $mnt for usr $me..."

# make directory and log mounting dirname
sudo -u $me sh << EOF
mkdir -p "$mnt"
echo "$mnt" > mount.log
EOF

# mount
mount -t reiserfs ./vdisk.img "$mnt"
