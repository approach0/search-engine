#!/bin/bash
tsv_file="$1"
while read l; do
	# "& 100 & Baseline & 548.30& 590.02& 370.00& 7.00& 2146.00& 440.25& 408.79& 234.00& 12.00& 1393.00\\"
	awk '{print "&", $2, "&", $4; for (i = 9; i <= 18; i++) printf("& %s", $i); printf("\\\\ \n") }' | sed -e 's/_/-/' -e 's/none/Baseline/' -e 's/maxref/MaxRef/' -e 's/gbp/GBP/' -e 's/len/LEN/' -e 's/num/NUM/'
done < $tsv_file
echo ""
