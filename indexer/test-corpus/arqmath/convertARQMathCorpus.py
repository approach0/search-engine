#!/usr/bin/python
import os
import json
import argparse
from xmlr import xmliter
from bs4 import BeautifulSoup

from replace_post_tex import replace_dollar_tex
from replace_post_tex import replace_display_tex
from replace_post_tex import replace_inline_tex

root_url = 'http://math.stackexchange.com/questions'
DIV = 500

def html2text(html):
	soup = BeautifulSoup(html, "html.parser")
	text = soup.text
	text = replace_display_tex(text)
	text = replace_inline_tex(text)
	text = replace_dollar_tex(text)
	return text


def saveSplit(ID, content, mode):
	folder = ID % 500
	directory = f'tmp/{folder}'
	os.system(f'mkdir -p {directory}')
	xml_path = f'{directory}/{ID}.txt'

	try:
		with open(xml_path, mode) as fh:
			fh.write(content)
	except:
		return

def each_file(root, extname):
	for dirname, dirs, files in os.walk(root):
		for f in files:
			if f.split('.')[-1] == extname:
				yield (dirname, f)

if __name__ == "__main__":
	parser = argparse.ArgumentParser(description='Convert ARQMath Dataset to text files grouped by threads.')
	parser.add_argument('--post-xml', type=str, help="Path to Posts.xml.", required=False)
	parser.add_argument('--2json', type=str, help="Convert thread text files to JSONs.", required=False)
	args = vars(parser.parse_args())

	if args['post_xml']:
		for attrs in xmliter(args['post_xml'], 'row'):
			postType = attrs['@PostTypeId']
			ID = int(attrs['@Id'])
			if '@Body' not in attrs:
				continue

			body = attrs['@Body']
			body = html2text(body)
			if postType == "1":
				print('thread', ID)
				title = attrs['@Title']
				title = html2text(title)
				# print(title, end="\n\n")
				saveSplit(ID, title + "\n\n" + body, 'w')
			else:
				parentID = int(attrs['@ParentId'])
				saveSplit(parentID, "\n\n" + body, 'a')

	if args['2json']:
		for dirname, fname in each_file(args['2json'], 'txt'):
			fpath = f"{dirname}/{fname}"
			opath = f"{dirname}/jj"
			with open(fpath) as fh:
				ID = fname.split('.')[0]
				text = fh.read()
				url = f"{root_url}/{ID}/?noredirect=1"
				with open(f"{dirname}/{ID}.json", "w") as fh_out:
					print('json', f"{dirname}/{ID}.json")
					fh_out.write(json.dumps({
						"url": url,
						"text": text
					}, sort_keys=True))
