#!/usr/bin/python
import os
import json
import csv
import argparse
from xmlr import xmliter
from bs4 import BeautifulSoup

from replace_post_tex import replace_dollar_tex
from replace_post_tex import replace_display_tex
from replace_post_tex import replace_inline_tex

root_url = 'http://math.stackexchange.com/questions'

def html2text(html):
	soup = BeautifulSoup(html, "html.parser")
	for elem in soup.select('span.math-container'):
		elem.replace_with('[imath]' + elem.text + '[/imath]')
	text = soup.text
	#text = replace_display_tex(text)
	#text = replace_inline_tex(text)
	#text = replace_dollar_tex(text)
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
	parser.add_argument('--task2', type=str, help="Path to Task-2 csv directory", required=False)
	parser.add_argument('--2json', type=str, help="Convert thread text files to JSONs.", required=False)
	args = vars(parser.parse_args())

	if args['task2']:
		for dirname, fname in each_file(args['task2'], 'tsv'):
			fpath = f"{dirname}/{fname}"
			print(f'Processing {fpath} ...')
			with open(fpath) as tsvfile:
				tsvreader = csv.reader(tsvfile, delimiter="\t")
				for i, line in enumerate(tsvreader):
					if i == 0:
						continue
					formulaID = line[0]
					topic_id = line[1] # or PostID
					thread_id = line[2]
					type_ = line[3]
					visual_id = line[4]
					latex = line[5]
					if type_  == 'comment':
						#print('skip comment')
						continue
					elif latex.isdigit():
						#print('skip single number')
						continue
					elif len(latex) == 1:
						#print('skip single letter TeX')
						continue
					#print(formulaID, topic_id, thread_id, latex)
					folder = int(formulaID) % 1200
					place = f'task2-corpus/{folder}'
					os.system(f'mkdir -p {place}')
					place = f'{place}/{formulaID}.json'
					with open(place, "w") as fh_out:
						#print('json', place)
						fh_out.write(json.dumps({
							"url": f"{formulaID},{topic_id},{thread_id}",
							"text": '[imath]' + latex + '[/imath]'
						}, sort_keys=True))

	if args['post_xml']:
		for attrs in xmliter(args['post_xml'], 'row'):
			postType = attrs['@PostTypeId']
			ID = int(attrs['@Id'])
			if '@Body' not in attrs:
				continue

			print('ID', ID)

			body = attrs['@Body']
			body = html2text(body)

			#if postType == "1":
			#	title = attrs['@Title']
			#	title = html2text(title)
			#	# print(title, end="\n\n")
			#	saveSplit(ID, title + "\n\n" + body, 'w')
			#else:
			if postType != "1":
				parentID = int(attrs['@ParentId'])
				saveSplit(ID, ' ' + "\n\n" + body, 'w')

			if ID == 554:
				print(body)
				#break

	if args['2json']:
		for dirname, fname in each_file(args['2json'], 'txt'):
			fpath = f"{dirname}/{fname}"
			with open(fpath) as fh:
				ID = fname.split('.')[0]
				text = fh.read()
				#url = f"{root_url}/{ID}/?noredirect=1"
				url = ID
				with open(f"{dirname}/{ID}.json", "w") as fh_out:
					print('json', f"{dirname}/{ID}.json")
					fh_out.write(json.dumps({
						"url": url,
						"text": text
					}, sort_keys=True))
