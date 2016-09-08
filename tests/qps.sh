#!/bin/bash
function show_progress() {
	# calculate time interval
	cur_t=`date +%s`
	t0=`cat "qps-t0.log"`
	let "t = cur_t - t0"

	tot=`wc -l qps-tot.log | awk '{print $1}'`
	suc=`wc -l qps-suc.log | awk '{print $1}'`
	echo "${t} sec: success rate: ${suc}/${tot}"
	if [ $t -ne 0 ]; then
		echo -n 'QPS: '
		awk "BEGIN {print ${suc}/${t}}"
	fi
}

function do_curl() {
	url="$1"
	curl "${url}" >> qps.log 2>> /dev/null && echo 'good' >> qps-suc.log
	echo 'done' >> qps-tot.log
}

#url='http://www.baidu.com/s?wd=%E6%B2%A1%E6%9C%89%E5%B9%B6%E4%B8%8D'
url='https://approach0.xyz/search/search-relay.php?p=1&q=%24%5Cmin(d(x%2Cy)%2C2)%24%2C%20metric%20space'

> qps.log
> qps-tot.log
> qps-suc.log

date +%s > "qps-t0.log"

for i in `seq 1 100`; do
	(do_curl "$url"; show_progress) &
done
