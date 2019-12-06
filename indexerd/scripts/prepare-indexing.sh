#!/bin/sh
INDEX_ROOT=~/nvme0n1/
INDEX_SIZE=4700 # in MiB
INDEXD_BIN=~/Desktop/search-engine-v3/v3/indexerd/run/indexerd.out
INDEX_NUMS=(0 1 2 3)

if [ "$1" == "-h" ]; then
cat << USAGE
Description:
Prepare indexing later to be fed by json-feeder.

$0 create-images
$0 mount-images (will prompt you for sudo password)
$0 umount-images (will prompt you for sudo password)
$0 create-index-sessions
$0 inspect-index-sessions
$0 stop-index-sessions
USAGE
exit
fi

[ $# -ne 1 ] && echo 'bad arg.' && exit
mode=$1

function prepare_image() {
	img_name="$1.img"
	cd "$INDEX_ROOT"
	echo "preparing $img_name ..."
	vdisk-creat.sh reiserfs $INDEX_SIZE
	mv vdisk.img "$img_name"
}

function mount_image() {
	img_name="$1.img"
	cd "$INDEX_ROOT"
	echo "mount $img_name ..."
	sudo vdisk-mount.sh reiserfs "$img_name"
}

function umount_image() {
	img_name="$1.img"
	mnt_dir_name="mnt-$1.img"
	cd "$INDEX_ROOT"
	echo "umount $img_name ..."
	sudo umount "$img_name"
	rmdir "$mnt_dir_name"
}

function spawn_indexd() {
	name="$1"
	port="$2"
	mnt_dir_name="mnt-$1.img"
	indexd_dir=$(dirname $INDEXD_BIN)
	indexd_bin=$(basename $INDEXD_BIN)
	if tmux has-session -t $name 2> /dev/null; then
		echo "sesseion $name already exists."
		return
	fi
	echo "starting $indexd_bin in tmux session $name ..."
	set -x
	tmux new-session -c "${indexd_dir}" -d -s $name \
		"./${indexd_bin} -o ${INDEX_ROOT}/${mnt_dir_name}/ -p $port 2> ./err-$name.log"
	set +x
}

if [ $mode == "create-images" ]; then
	for i in "${INDEX_NUMS[@]}"; do
		prepare_image index-$i
	done;

elif [ $mode == "mount-images" ]; then
	for i in "${INDEX_NUMS[@]}"; do
		mount_image index-$i
	done;

elif [ $mode == "umount-images" ]; then
	for i in "${INDEX_NUMS[@]}"; do
		umount_image index-$i
	done;

elif [ $mode == "create-index-sessions" ]; then
	for i in "${INDEX_NUMS[@]}"; do
		spawn_indexd  index-$i "890${i}"
	done;

elif [ $mode == "inspect-index-sessions" ]; then
	while true; do
		tmux list-sessions | grep "index-[0-9]\+"
		for i in "${INDEX_NUMS[@]}"; do
			name=index-$i
			tmux capture-pane -pt $name
			echo "[ session $name ]"
			sleep 1;
		done;
	done;

elif [ $mode == "stop-index-sessions" ]; then
	for i in "${INDEX_NUMS[@]}"; do
		echo "stop session index-$i"
		tmux send-keys -t index-$i  C-c
	done;
fi;
