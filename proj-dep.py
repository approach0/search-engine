from os import listdir
from os import path
import re
import getopt, sys

def pri_targets_mk(targets_file):
	targets_li = []
	files = listdir('./')
	for d in files:
		if path.isdir(d) and path.exists(d + '/Makefile'):
			targets_li.append(d)
	targets = ' '.join(targets_li)
	print(targets)


def pri_dep_mk(targets_file):
	targets_str = open(targets_file).read().strip()
	targets_li = targets_str.split()
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
		if len(dep_li):
			for dep in dep_li:
				print('{}-module: {}-module'.format(target, dep))

def pri_dep_dot(targets_file):
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
				save_extdep_li.append('"{}"[shape="box"]'.format(libdep))
				print('\t"{}" -> "{}"'.format(target, libdep))

	# write rank constraint for external module
	print('\t{')
	print('\t\trank = same;')
	for save in save_extdep_li:
		print('\t\t' + save)
	print('\t}')

	print("}") # end of graph

def help(arg0):
	print('DESCRIPTION: generate project dependency .mk or .dot.' \
	      '\n' \
	      'SYNOPSIS:\n' \
	      '%s [-h | --help] ' \
	      '[--dot] ' \
	      '[--dep]' \
	      '[--targets]' \
	      '\n' \
	      % (arg0))
	sys.exit(1)

def main():
	config_targets_file = "targets.mk"
	cmd = sys.argv[0]
	args = sys.argv[1:]
	try:
		opts, args = getopt.getopt(sys.argv[1:], "h",
		                           ['help', 'dot', 'dep', 'targets'])
	except:
		help(cmd)

	if len(opts) == 0:
		help(cmd)

	for opt, arg in opts:
		if opt in ("-h", "--help"):
			help(cmd)
		if opt in ("--dot"):
			pri_dep_dot(config_targets_file)
			break
		if opt in ("--dep"):
			pri_dep_mk(config_targets_file)
			break
		if opt in ("--targets"):
			pri_targets_mk(config_targets_file)
			break

if __name__ == "__main__":
	main()
