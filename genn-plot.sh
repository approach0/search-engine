#!/bin/sh

# example: $0 ./tmp/cikm-3.dat ./tmp/STD-MC-0.3.dat

data1=$1
data2=$2
output_dir='tmp'

evalscript=./eval-trec-results-fullrel.sh
pipescript=./plot_scripts/pipe.sh
plot_py_template=./plot_scripts/plot.py

dat1_queries_bpref=`$evalscript $data1 | $pipescript`
dat1_overall_bpref=`$evalscript $data1 | grep 'bpref' | grep -m 1 'all' | awk '{print $3}'`

dat2_queries_bpref=`$evalscript $data2 | $pipescript`
dat2_overall_bpref=`$evalscript $data2 | grep 'bpref' | grep -m 1 'all' | awk '{print $3}'`

dat1_bprefs="$dat1_queries_bpref $dat1_overall_bpref"
dat2_bprefs="$dat2_queries_bpref $dat2_overall_bpref"

echo $dat1_overall_bpref
echo $dat2_overall_bpref

label=`basename $data2`
sed "s/__dat1__/$dat1_bprefs/; s/__dat2__/$dat2_bprefs/; s/__label__/$label/g; s/__prefix__/$output_dir/;" $plot_py_template | python3
