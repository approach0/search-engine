#!/usr/bin/python3
import sys
import statistics
import json

runtime02 = []
runtime24 = []
runtime04 = []

for runtime_file in sys.argv[1:]:
    runtime = []
    with open(runtime_file, 'r') as runtime_fh:
        for line in runtime_fh:
            line = line.rstrip()
            fields = line.split(' ')
            runtime.append(float(fields[1]))
    runtime02 += runtime[0:20]
    runtime24 += runtime[20:40]
    runtime04 += runtime[:]

output = {}
output['avg-02'] = statistics.mean(runtime02)
output['med-02'] = statistics.median(runtime02)
output['max-02'] = max(runtime02)
output['min-02'] = min(runtime02)
output['std-02'] = statistics.stdev(runtime02)

output['avg-24'] = statistics.mean(runtime24)
output['med-24'] = statistics.median(runtime24)
output['max-24'] = max(runtime24)
output['min-24'] = min(runtime24)
output['std-24'] = statistics.stdev(runtime24)

output['avg-04'] = statistics.mean(runtime04)
output['med-04'] = statistics.median(runtime04)
output['max-04'] = max(runtime04)
output['min-04'] = min(runtime04)
output['std-04'] = statistics.stdev(runtime04)

output['runs'] = [len(runtime02), len(runtime24), len(runtime04)]
output['runtimes'] = runtime04[:]
output_json = json.dumps(output, sort_keys=True, indent=4)
print(output_json)
