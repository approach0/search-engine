#!/usr/bin/python3
import sys
import statistics
import json

runtime_dat = sys.argv[1]
with open(runtime_dat, 'r') as runtime_fh:
    runtime = []
    for line in runtime_fh:
        line = line.rstrip()
        fields = line.split(' ')
        runtime.append(float(fields[1]))
    output = {}
    output['mean'] = statistics.mean(runtime)
    output['median'] = statistics.median(runtime)
    output['max'] = max(runtime)
    output['min'] = min(runtime)
    output['variance'] = statistics.variance(runtime)
    output['runtime'] = runtime
    output_json = json.dumps(output, sort_keys=True, indent=4)
    print(output_json)
