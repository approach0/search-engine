#!/usr/bin/python3
import statistics

with open('invlist-len.txt') as fh:
    lines = fh.readlines()
    numbers = list(map(lambda x: int(x) / 12, lines))
    output = {}
    output['avg'] = statistics.mean(numbers)
    output['med'] = statistics.median(numbers)
    output['max'] = max(numbers)
    output['min'] = min(numbers)
    output['std'] = statistics.stdev(numbers)
    print(output)
