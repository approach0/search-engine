from os import listdir
from os import path
from os import system
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

def mk_dep_dict(targets_file):
	dep_dict = dict()
	targets_str = open(targets_file).read().strip()
	targets_li = targets_str.split()
	for target in targets_li:
		dep_path = target + '/' + 'dep'
		if not path.exists(target) or not path.exists(dep_path):
			continue
		files = listdir(dep_path)
		regex = re.compile("dep-(.*)\.mk")
		if len(files) > 0:
			dep_dict[target] = list()
		for name in files:
			m = regex.match(name)
			if m is not None:
				dep = m.group(1)
				if path.exists(dep):
					dep_dict[target].append((dep, 'internal'))
				else:
					dep_dict[target].append((dep, 'external'))
	return dep_dict

def check_dep(targets_file):
	dd = mk_dep_dict(targets_file)
	flag = 0
	for module in dd:
		for dep, dep_type in dd[module]:
			if dep_type == 'external':
				print("checking lib" + dep + ' path', end="")
				print(' used by ' + module + '...')
				res = system('make -C %s check-lib%s'
				             ' > /dev/null' % (module, dep))
				if res != 0:
					flag = 1
					print("\033[91m", end="")
					print("linker cannot locate lib" + dep)
					print("\033[0m", end="")
	if flag == 0:
		print("all libraries are located, type `make' to build.")
	else:
		print("\033[91m", end="")
		print("some libraries are not found, building will fail.")
		print("\033[0m", end="")

def pri_carryalone_dep(dd, included, module, _type):
	if module not in dd:
		return
	dep_arr = dd[module]
	for dep, dep_type in dep_arr:
		if dep_type == _type:
			if dep not in included:
				included[dep] = True
				print(dep, end=" ")
		pri_carryalone_dep(dd, included, dep, _type)

def pri_proj_dep(targets_file):
	dd = mk_dep_dict(targets_file)
	for module in dd:
		print('%s.carryalone-extdep :=' % module, end=" ")
		included = dict()
		pri_carryalone_dep(dd, included, module, 'external')
		print('')

		print('%s.carryalone-intdep :=' % module, end=" ")
		included = dict()
		pri_carryalone_dep(dd, included, module, 'internal')
		print('')
	print('')

	for module in dd:
		dep_arr = dd[module]
		if len(dep_arr) > 0:
			print('%s-module: ' % (module), end="")
		for dep, dep_type in dep_arr:
			if (dep_type == 'internal'):
				print('%s-module' % (dep), end=" ")
		print("")

def pri_dep_dot(targets_file):
	dd = mk_dep_dict(targets_file)
	extern_dep_li = set()
	# begin graph
	print("digraph G {")
	for module in dd:
		print('\t"%s"' % (module))
		dep_arr = dd[module]
		for dep, dep_type in dep_arr:
			if dep_type == 'external':
				libdep = 'lib' + dep
				print('\t"%s"[shape="box"]' % (libdep))
				print('\t"%s" -> "%s"' % (module, libdep))
				extern_dep_li.add(libdep)
			else:
				print('\t"%s" -> "%s"' % (module, dep))

	# write rank constraint for external modules
	print('\t{')
	print('\t\trank = same;')
	for libdep in extern_dep_li:
		print('\t\t"%s"' % (libdep))
	print('\t}')

	# end of graph
	print("}")

def help(arg0):
	print('DESCRIPTION: generate project dependency .mk or .dot.' \
	      '\n' \
	      'SYNOPSIS:\n' \
	      '%s [-h | --help] ' \
	      '[--dot] ' \
	      '[--proj-dep]' \
	      '[--targets]' \
	      '[--check-dep]' \
	      '\n' \
	      % (arg0))
	sys.exit(1)

def main():
	config_targets_file = "targets.mk"
	cmd = sys.argv[0]
	args = sys.argv[1:]
	try:
		opts, args = getopt.getopt(sys.argv[1:], "h", [
			'help', 'dot', 'proj-dep', 'targets', 'check-dep'
		])
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
		if opt in ("--proj-dep"):
			pri_proj_dep(config_targets_file)
			break
		if opt in ("--targets"):
			pri_targets_mk(config_targets_file)
			break
		if opt in ("--check-dep"):
			check_dep(config_targets_file)
			break

if __name__ == "__main__":
	main()
