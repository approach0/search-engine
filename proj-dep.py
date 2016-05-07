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

def mk_dep_list(targets_file):
	targets_str = open(targets_file).read().strip()
	targets_li = targets_str.split()
	for target in targets_li:
		dep_path = target + '/' + 'dep'
		if not path.exists(target) or not path.exists(dep_path):
			continue
		files = listdir(target + '/dep')
		regex = re.compile("dep-(.*)\.mk")
		for name in files:
			m = regex.match(name)
			if m is not None:
				dep = m.group(1)
				if path.exists(dep):
					yield (target, dep, 'internal')
				else:
					yield (target, dep, 'external')

def pri_dep_mk(targets_file):
	for module, dep, dep_type in mk_dep_list("targets.mk"):
		if dep_type == 'internal':
			print('{}-module: {}-module'.format(module, dep))

def pri_dep_dot(targets_file):
	# begin graph
	print("digraph G {")

	for module, dep, dep_type in mk_dep_list("targets.mk"):
		print('\t"{}"'.format(module))
		if dep_type == 'external':
			libdep = 'lib' + dep
			print('\t"{}"[shape="box"]'.format(libdep))
			print('\t"{}" -> "{}"'.format(module, libdep))
		else:
			print('\t"{}" -> "{}"'.format(module, dep))

	# write rank constraint for external modules
	print('\t{')
	print('\t\trank = same;')
	for module, dep, dep_type in mk_dep_list("targets.mk"):
		if dep_type == 'external':
			libdep = 'lib' + dep
			print('\t\t' + '"{}"'.format(libdep))
	print('\t}')

	# end of graph
	print("}")

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
