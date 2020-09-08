#!/usr/bin/python3
from py_asciimath.translator.translator import MathML2Tex

import sys
import json
import os
import re
import errno

DIV = 100
mathml2tex = MathML2Tex()

def repl_callbk(m):
	mml = m.group(1)
	tex = mathml2tex.translate(mml, network=False, from_file=False)
	tex = '[imath]' + tex[1:-1] + '[/imath]'
	return tex


def process(txt):
	txt = re.sub("(<math.*?</math>)", repl_callbk, txt)
	print(txt)
	return txt


def mkdir_p(path):
	try:
		os.makedirs(path)
	except OSError as e:
		if e.errno == errno.EEXIST and os.path.isdir(path):
			pass
		else:
			raise Exception("mkdir needs permission")


def convert(path):
	abs_path = os.path.abspath(path)
	with open(abs_path, encoding='utf-8', errors='replace') as fh:
		cnt = 0;
		for line in fh.readlines():
			fields = line.strip().split(" ", 1)
			name = fields[0]
			ID = int(name[2:])
			txt = fields[1]
			j = {
				"url": name,
				"text": process(txt)
			}
			enc = json.dumps(j, ensure_ascii=False)

			outpath = './tmp/' + str(ID % DIV)
			mkdir_p(outpath)

			outfile = outpath+ f'/{name}.json'
			with open(outfile, "w") as outfh:
				outfh.write(enc)

			break ######
			cnt += 1


def help(arg0):
	print("usage: %s <path/file>" % arg0)
	sys.exit(1)


def main(arg):
	if len(arg) <= 1:
		help(arg[0])
	path = arg[1]
	convert(path)


if __name__ == "__main__":
	main(sys.argv)
