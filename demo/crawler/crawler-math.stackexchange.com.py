#!/usr/bin/python3
import time
import pycurl
import os
import errno
import code
import re
import json
import sys
import getopt
from replace_post_tex import replace_dollar_tex
from replace_post_tex import replace_display_tex
from replace_post_tex import replace_inline_tex
from io import BytesIO
from bs4 import BeautifulSoup

root_url = "http://math.stackexchange.com"
vt100_WARNING = '\033[93m'
vt100_RESET = '\033[0m'
DIVISIONS = 500

def print_err(err_str):
	f = open("error.log", "a")
	print(vt100_WARNING)
	err_str = "[error] " + err_str
	f.write(err_str + '\n')
	print(err_str)
	print(vt100_RESET)
	f.close()

def curl(sub_url, c):
	buf = BytesIO()
	print('[curl] %s' % sub_url)
	url = root_url + sub_url
	url = url.encode('iso-8859-1')
	c.setopt(c.URL, url)
	c.setopt(c.WRITEFUNCTION, buf.write)
	errs = 0
	while True:
		try:
			c.perform()
		except (KeyboardInterrupt, SystemExit):
			print('user aborting...')
			raise
		except:
			errs = errs + 1
			if errs > 3:
				buf.close()
				raise
			print("[curl] try again...")
			continue
		break
	res_str = buf.getvalue()
	buf.close()
	return res_str

def extract_p_tag_text(soup, id_str):
	txt = ''
	id_tag = soup.find(id=id_str)
	if id_tag is None:
		raise Exception("id_tag is None")
	p_tags = id_tag.find_all('p')
	for p in p_tags:
		if p.string is not None:
			txt += str(p.string)
			txt += '\n'
	return txt

def crawl_post_page(sub_url, c):
	try:
		post_page = curl(sub_url, c)
	except:
		raise
	s = BeautifulSoup(post_page, "html.parser")
	# get header/title
	question_header = s.find(id='question-header')
	if question_header is None:
		raise
	header = str(question_header.h1.string)
	post_txt = header + '\n\n'
	# get question
	try:
		post_txt += extract_p_tag_text(s, "question")
	except:
		raise
	post_txt += '\n'
	# get answers
	try:
		post_txt += extract_p_tag_text(s, "answers")
	except:
		raise
	return post_txt

def mkdir_p(path):
	try:
		os.makedirs(path)
	except OSError as e:
		if e.errno == errno.EEXIST and os.path.isdir(path):
			pass
		else:
			raise Exception("mkdir needs permission")

def save_preview(path, post_txt, url):
	# put preview into HTML template
	f = open("template.html", "r")
	fmt_str = f.read()
	f.close()
	post_txt = post_txt.replace("\n", "</br>");
	preview = fmt_str.replace("{PREVIEW}", post_txt);
	preview = preview.replace("{URL}", url);
	# save preview
	f = open(path, "w")
	f.write(preview)
	f.close()

def save_json(path, post_txt, url):
	f = open(path, "w")
	f.write(json.dumps({
		"url": url,
		"text": post_txt
	}))
	f.close()

def get_curl():
	c = pycurl.Curl()
	c.setopt(c.CONNECTTIMEOUT, 8)
	c.setopt(c.TIMEOUT, 10)

	# redirect on 3XX error
	c.setopt(c.FOLLOWLOCATION, 1)
	return c

def list_post_links(page, c):
	links = list()
	sub_url = '/questions?sort=newest&page={}'.format(page)
	try:
		navi_page = curl(sub_url, c)
	except Exception as e:
		yield (None, None, e)
	s = BeautifulSoup(navi_page, "html.parser")
	summary_tags = s.find_all('div', {"class": "question-summary"})
	for div in summary_tags:
		a_tag = div.find('a', {"class": "question-hyperlink"})
		if not div.has_attr('id') or not a_tag.has_attr('href'):
			continue
		yield (div['id'], a_tag['href'], None)

def get_file_path(post_id):
	directory = './tmp/' + str(post_id % DIVISIONS)
	return directory + '/' + str(post_id)

def process_post(post_id, post_txt, url):
	# decide sub-directory
	file_path = get_file_path(post_id)
	try:
		mkdir_p(os.path.dirname(file_path))
	except:
		raise
	# process TeX mode pieces
	post_txt = replace_display_tex(post_txt)
	post_txt = replace_inline_tex(post_txt)
	post_txt = replace_dollar_tex(post_txt)
	# save files
	save_json(file_path + '.json',
				 post_txt, url)
	save_preview(file_path + '.html',
				 post_txt, url)

def crawl_pages(start, end):
	c = get_curl()
	for page in range(start, end + 1):
		print("[page] %d / %d" % (page, end))
		for div_id, sub_url, e in list_post_links(page, c):
			if e is not None:
				print_err("page %d" % page)
				break
			res = re.search('question-summary-(\d+)', div_id)
			if not res:
				print_err("div ID %s" % div_id)
				continue
			ID = int(res.group(1))
			file_path = get_file_path(ID);
			if os.path.isfile(file_path + ".json"):
				print('[already exists] ' + file_path)
				continue
			try:
				url = root_url + sub_url
				post_txt = crawl_post_page(sub_url, get_curl())
				process_post(ID, post_txt, url)
			except (KeyboardInterrupt, SystemExit):
				print('[abort]')
				return
			except:
				print_err("post %s" % url)
				continue
			time.sleep(0.6)

def help(arg0):
	print('DESCRIPTION: crawler script for math.stackexchange.com.' \
	      '\n\n' \
	      'SYNOPSIS:\n' \
	      '%s [-b | --begin-page <page>] ' \
	      '[-e | --end-page <page>] ' \
	      '[-p | --post <post id>] ' \
	      '\n' % (arg0))
	sys.exit(1)

def main(arg):
	argv = arg[1:]
	try:
		opts, args = getopt.getopt(
			argv, "b:e:p:",
			['begin-page=', 'end-page=', 'post=']
		)
	except:
		help(arg[0])

	begin_page = 1;
	end_page = -1;
	for opt, arg in opts:
		if opt in ("-b", "--begin-page"):
			begin_page = int(arg);
			continue
		if opt in ("-e", "--end-page"):
			end_page = int(arg);
			continue
		if opt in ("-p", "--post"):
			file_path = get_file_path(int(arg));
			if os.path.isfile(file_path + ".json"):
				print('[already exists] ' + file_path)
				return
			sub_url = "/questions/" + arg
			full_url = root_url + sub_url
			post_txt = crawl_post_page(sub_url, get_curl())
			process_post(int(arg), post_txt, full_url)
			return

	if (end_page >= begin_page):
		crawl_pages(begin_page, end_page)
	else:
		help(arg[0])

if __name__ == "__main__":
	main(sys.argv)
