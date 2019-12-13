#!/bin/sh
function do_txt2json_in () {
	echo "converting text file to json in directory ${1}..."
	find "$1" -type f -name '*.blog' | xargs -I {} ./txt2json.py {}
}

do_txt2json_in ~/nvme0n1/test-corpus
