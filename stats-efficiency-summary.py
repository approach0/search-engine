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

    runtime20 = runtime[0:20]
    output['mean20'] = statistics.mean(runtime20)
    output['median20'] = statistics.median(runtime20)
    output['max20'] = max(runtime20)
    output['min20'] = min(runtime20)
    output['std20'] = statistics.stdev(runtime20)

    output['mean'] = statistics.mean(runtime)
    output['median'] = statistics.median(runtime)
    output['max'] = max(runtime)
    output['min'] = min(runtime)
    output['std'] = statistics.stdev(runtime)
    output['runtime'] = runtime
    output_json = json.dumps(output, sort_keys=True, indent=4)
    print(output_json)
