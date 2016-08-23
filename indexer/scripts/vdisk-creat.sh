#!/bin/sh
dd if=/dev/zero of=vdisk.img count=2 bs=1024M

mkfs.reiserfs -ff ./vdisk.img
