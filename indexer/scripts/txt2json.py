#!/usr/bin/python3
import sys
import json
import os

def convert(path):
	abs_path = os.path.abspath(path)
	print("converting:\n %s" % abs_path)
	# open file with erros='replace' option will replace the character
	# that cannot be decoded to a question mark symbol (i.e. "�")
	txt = open(path, encoding='utf-8', errors='replace').read()

	j = {
		"url": "file://" + abs_path,
		"text": txt
	}
	enc = json.dumps(j, ensure_ascii=False)

	new_file = open(path + '.json', "w")
	new_file.write(enc)

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
