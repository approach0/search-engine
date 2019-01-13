#!/bin/bash
if [ "$1" == "-h" ]; then
cat << USAGE
Examples:
cat docID.log | $0 /path/to/index
USAGE
exit
fi

[ $# -ne 1 ] && echo 'bad arg.' && exit

index_idr="${1}"
script_dir=`cd "$(dirname "${BASH_SOURCE[0]}")" && pwd`

function get_url() {
	docID="$1"
	cd $script_dir/../../indices
	./run/doc-lookup.out -p "$index_idr" -i "$docID" | grep 'URL:' | awk '{print $2}'
}

cat /dev/stdin | while read line; do
	info=`echo $line | awk '{print $1}'`
	docID=`echo $line | awk '{print $2}'`
	echo -n "$info "
	get_url $docID
done
