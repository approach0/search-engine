#!/bin/bash
find ~/Desktop/index-ntcir12/prefix/ -name 'posting.bin' | while read line; do
	bytes=$(du -b $line | awk '{print $1}')
	echo $bytes
done
