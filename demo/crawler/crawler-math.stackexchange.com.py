#!/usr/bin/python3
import time
import pycurl
import os
import errno
import certifi
import code
import re
import json
import sys
import getopt
import filecmp
from replace_post_tex import replace_dollar_tex
from replace_post_tex import replace_display_tex
from replace_post_tex import replace_inline_tex
from io import BytesIO
from bs4 import BeautifulSoup

root_url = "http://math.stackexchange.com"
vt100_BLUE = '\033[94m'
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

def extract_p_tag_text(soup):
	txt = ''
	p_tags = soup.find_all('p')
	for p in p_tags:
		if p.text is not ' ':
			txt += str(p.text)
			txt += '\n'
	return txt

def extract_comments_text(soup):
	txt = ''
	div_tags = soup.find_all('div', class_="comments")
	for div in div_tags:
		span_tags = div.find_all('span', class_="comment-copy")
		for span in span_tags:
			txt += str(span.text)
			txt += '\n'
	return txt

def crawl_post_page(sub_url, c):
	try:
		post_page = curl(sub_url, c)
	except:
		raise
	s = BeautifulSoup(post_page, "html.parser")
	# get title
	question_header = s.find(id='question-header')
	if question_header is None:
		raise Exception("question header is None")
	title = str(question_header.h1.string)
	post_txt = title + '\n\n'
	# get question
	question = s.find(id="question")
	if question is None:
		raise Exception("question tag is None")
	post_txt += extract_p_tag_text(question)
	post_txt += '\n'
	# get question comments
	post_txt += extract_comments_text(question)
	post_txt += '\n'

	# get answers
	answers = s.find(id="answers")
	if answers is None:
		raise Exception("answers tag is None")
	for answer in answers.findAll('div',{'class':'answer'}):
		post_txt += extract_p_tag_text(answer)
		post_txt += '\n'
		# get answer comments
		post_txt += extract_comments_text(answer)
		post_txt += '\n'
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
	post_txt = post_txt.replace("\n", "</br>")
	preview = fmt_str.replace("{PREVIEW}", post_txt)
	preview = preview.replace("{URL}", url)
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
	c.setopt(pycurl.CAINFO, certifi.where())

	# redirect on 3XX error
	c.setopt(c.FOLLOWLOCATION, 1)
	return c

def list_post_links(page, sortby, c):
	# sortby can be 'newest', 'active' etc.
	sub_url = '/questions?sort={}&page={}'.format(sortby, page)

	try:
		navi_page = curl(sub_url, c)
	except Exception as e:
		yield (None, None, e)
	s = BeautifulSoup(navi_page, "html.parser")
	summary_tags = s.find_all('div', {"class": "question-summary"})
	for div in summary_tags:
		a_tag = div.find('a', {"class": "question-hyperlink"})
		if a_tag is None:
			continue
		elif not div.has_attr('id') or not a_tag.has_attr('href'):
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

	# do not touch time stamp if previously
	# an identical file already exists.
	jsonfile = file_path + ".json"
	if os.path.isfile(jsonfile):
		print('[exists]')
		save_json('/tmp/tmp.json', post_txt, url)
		if filecmp.cmp('/tmp/tmp.json', jsonfile):
			# two files are identical, do not touch
			print('[identical, no touch]')
			return
		else:
			print('[overwrite]')

	# two files are different, save files
	save_json(jsonfile, post_txt, url)
	save_preview(file_path + '.html', post_txt, url)

def crawl_pages(sortby, start, end, extra_opt):
	c = get_curl()
	for page in range(start, end + 1):
		print(vt100_BLUE)
		print("[page] %d / %d order by %s" % (page, end, sortby))
		print(vt100_RESET)
		succ_posts = 0
		for div_id, sub_url, e in list_post_links(page, sortby, c):
			if e is not None:
				print_err("page %d" % page)
				break
			res = re.search('question-summary-(\d+)', div_id)
			if not res:
				print_err("div ID %s" % div_id)
				continue
			ID = int(res.group(1))
			file_path = get_file_path(ID)
			if os.path.isfile(file_path + ".json"):
				if not extra_opt["overwrite"]:
					print('[exists, skip] ' + file_path)
					# count on success
					succ_posts += 1
					continue
			try:
				url = root_url + sub_url
				post_txt = crawl_post_page(sub_url, get_curl())
				process_post(ID, post_txt, url)
			except (KeyboardInterrupt, SystemExit):
				print('[abort]')
				return 'abort'
			except:
				print_err("post %s" % url)
				continue

			# count on success
			succ_posts += 1

			# sleep to avoid over-frequent request.
			time.sleep(0.6)

		# log crawled page number
		page_log = open("page.log", "a")
		page_log.write('page %s: %d posts successful.\n' %
		               (page, succ_posts))
		page_log.close()
	return 'finish'

def help(arg0):
	print('DESCRIPTION: crawler script for math.stackexchange.com.' \
	      '\n\n' \
	      'SYNOPSIS:\n' \
	      '%s [-b | --begin-page <page>] ' \
	      '[-e | --end-page <page>] ' \
	      '[--no-overwrite] ' \
	      '[--patrol] ' \
	      '[--hook-script <script name>] ' \
	      '[-p | --post <post id>] ' \
	      '\n' % (arg0))
	sys.exit(1)

def main(args):
	argv = args[1:]
	try:
		opts, _ = getopt.getopt(
			argv, "b:e:p:h", [
				'begin-page=',
				'end-page=',
				'post=',
				'no-overwrite',
				'patrol',
				'hook-script='
			]
		)
	except:
		help(args[0])

	# default arguments
	extra_opt = {
		"overwrite": True,
		"hookscript": "",
		"patrol": False
	}
	begin_page = 1
	end_page = -1

	for opt, arg in opts:
		if opt in ("-b", "--begin-page"):
			begin_page = int(arg)
			continue
		elif opt in ("-e", "--end-page"):
			end_page = int(arg)
			continue
		elif opt in ("-p", "--post"):
			sub_url = "/questions/" + arg
			full_url = root_url + sub_url
			post_txt = crawl_post_page(sub_url, get_curl())
			process_post(int(arg), post_txt, full_url)
			exit(0)
		elif opt in ("--no-overwrite"):
			extra_opt["overwrite"] = False
		elif opt in ("--patrol"):
			extra_opt["patrol"] = True
		elif opt in ("--hook-script"):
			extra_opt["hookscript"] = arg
		else:
			help(args[0])

	if (end_page >= begin_page):
		while True:
			# crawling newest pages
			r = crawl_pages('newest', begin_page, end_page,
			                extra_opt)
			if r == 'abort':
				break

			# if patrol mode is enabled, also crawl recently active
			# posts.
			if extra_opt['patrol']:
				# crawling recently active pages
				r = crawl_pages('active', begin_page, end_page,
								extra_opt)
				if r == 'abort':
					break

			# now it is the time to invoke hookscript.
			if extra_opt["hookscript"]:
				os.system(extra_opt["hookscript"])

			if extra_opt['patrol']:
				# if patrol mode is enabled, repeatedly crawl
				# the page range instead of breaking out of loop.
				pass
			else:
				break
	else:
		help(args[0])

if __name__ == "__main__":
	main(sys.argv)
