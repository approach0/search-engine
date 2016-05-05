from os import listdir
from os import path 
import re

targets_file = "targets.mk"
output_dot_file = "dep.dot"

targets_str = open(targets_file).read().strip()
targets_li = targets_str.split()
save_extdep_li = []

print("digraph G {") # begin graph

for target in targets_li:
	if not path.exists(target):
		continue
	files = listdir(target)
	regex = re.compile("dep-(.*)\.mk")
	dep_li = []
	ext_dep_li = []
	for name in files:
		m = regex.match(name)
		if m is not None:
			dep = m.group(1)
			if path.exists(dep):
				dep_li.append(dep)
			else:
				ext_dep_li.append(dep)
	print('\t"{}"'.format(target))
	if len(dep_li):
		for dep in dep_li:
			print('\t"{}" -> "{}"'.format(target, dep))
	if len(ext_dep_li):
		for dep in ext_dep_li:
			libdep = 'lib' + dep
			# print('\t"{}"[shape="box"]'.format(libdep))
			save_extdep_li.append('"{}"[shape="box"]'.format(libdep));
			print('\t"{}" -> "{}"'.format(target, libdep))

# write rank constraint for external module
print('\t{')
print('\t\trank = same;')
for save in save_extdep_li:
	print('\t\t' + save)
print('\t}')

print("}") # end of graph
