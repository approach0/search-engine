#!/bin/bash
tsv_file="$1"
while read l; do
	awk '{print $6, "&", $4 "-" $2; for (i = 9; i <= 18; i++) printf("& %s", $i); printf("\\\\ \n") }' | sed -e 's/_/-/' -e 's/none/None/' -e 's/maxref/MaxRef/' -e 's/gbp/GBP/' -e 's/len/LEN/' -e 's/num/NUM/' -e 's/wiki/Wiki/' -e 's/full-mse/MSE/'
done < $tsv_file
echo ""
