#!/usr/bin/python3
import time
import pycurl
import os
import errno
import json
import sys
import getopt
import filecmp
import certifi
from urllib.parse import urlencode
from replace_post_tex import replace_dollar_tex
from replace_post_tex import replace_display_tex
from replace_post_tex import replace_inline_tex
from replace_post_tex import replace_canonical_tex
from slimit import ast
from slimit.parser import Parser
from io import BytesIO
from bs4 import BeautifulSoup

root_url = "https://artofproblemsolving.com"
vt100_BLUE = '\033[94m'
vt100_WARNING = '\033[93m'
vt100_RESET = '\033[0m'
DIVISIONS = 500

# load parser globally for sharing, it is painfully slow
slimit_parser = Parser()

def print_err(err_str):
    f = open("error.log", "a")
    print(vt100_WARNING)
    err_str = "[error] " + err_str
    f.write(err_str + '\n')
    print(err_str)
    print(vt100_RESET)
    f.close()

def curl(sub_url, c, post = None):
    buf = BytesIO()
    print('[curl] %s' % sub_url)
    url = root_url + sub_url
    url = url.encode('iso-8859-1')
    c.setopt(c.URL, url)
    c.setopt(c.WRITEFUNCTION, buf.write)
    #c.setopt(c.VERBOSE, True)
    if post is not None:
        c.setopt(c.POST, 1)
        c.setopt(c.POSTFIELDS, urlencode(post))
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

def parse_op_name(obj):
    if isinstance(obj, ast.DotAccessor):
        l = obj.node.value + "." + obj.identifier.value
    elif isinstance(obj, ast.String):
        l = obj.value
        l = l.strip('"')
        l = l.encode().decode('unicode_escape')
    elif hasattr(obj, 'value'):
        l = obj.value
    else:
        l = "<UnknownName>"
    return l

def parse_node(node):
    ret = {}
    if hasattr(node, 'value') or isinstance(node, ast.DotAccessor):
        return parse_op_name(node)
    if isinstance(node, ast.Object):
        for prop in node.properties:
            l = parse_op_name(prop.left)
            r = parse_node(prop.right)
            ret[l] = r
        return ret
    elif isinstance(node, ast.Array):
        list = []
        for child in node:
            list.append(parse_node(child))
        return list
    elif isinstance(node, ast.FunctionCall):
        return "<FunctionCall>"
    elif isinstance(node, ast.FuncExpr):
        return "<FuncExpr>"
    elif isinstance(node, ast.Program):
        for child in node:
            if isinstance(child, ast.ExprStatement):
                expr = child.expr
                if isinstance(expr, ast.Assign):
                    l = parse_op_name(expr.left)

                    ret[l] = parse_node(expr.right)
    else:
        return "<UnknownRight>"
    return ret

def get_bootstrap_data(page):
    s = BeautifulSoup(page, "html.parser")
    parser = slimit_parser
    for script in s.findAll('script'):
        # parse all assignments
        if 'AoPS.bootstrap_data' in script.text:
            try:
                tree = parser.parse(script.text)
                parsed = parse_node(tree)
                return parsed
            except SyntaxError:
                return None

    return None

def crawl_post_page(sub_url, c):
    try:
        post_page = curl(sub_url, c)
    except:
        raise

    parsed = get_bootstrap_data(post_page)
    topic_data = parsed['AoPS.bootstrap_data']['preload_cmty_data']['topic_data']

    # get title
    title = topic_data['topic_title']
    post_txt = title + '\n\n'

    # get posts
    for post in topic_data['posts_data']:
        post_txt += post['post_canonical']
        post_txt += '\n\n'

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
    c.setopt(pycurl.CAINFO, certifi.where())
    c.setopt(c.CONNECTTIMEOUT, 8)
    c.setopt(c.TIMEOUT, 10)
    c.setopt(c.COOKIEJAR, 'cookie.txt')
    c.setopt(c.COOKIEFILE, 'cookie.txt')

    # redirect on 3XX error
    c.setopt(c.FOLLOWLOCATION, 1)
    return c

def list_category_links(category, newest, oldest, c):
    #first access the page to acquire session id
    sub_url = '/community/'

    community_page = curl(sub_url, c)

    parsed = get_bootstrap_data(community_page)
    session = parsed['AoPS.session']
    session_id = session['id']
    user_id = session['user_id']
    server_time = int(parsed['AoPS.bootstrap_data']['init_time'])

    oldest_post_time = server_time - oldest * 24 * 60 * 60
    newest_post_time = server_time - newest * 24 * 60 * 60

    fetch_after = newest_post_time
    while fetch_after >= oldest_post_time:
        print(vt100_BLUE)
        print("[category] %d , [before-time] %s " %
              (category, time.strftime('%Y-%m-%d %H:%M:%S', time.gmtime(fetch_after))))
        print(vt100_RESET)
        sub_url = '/m/community/ajax.php'

        postfields = {"category_type": "forum",
                      "log_visit": 1,
                      "required_tag": "",
                      "fetch_before": fetch_after,
                      "user_id": 0,
                      "fetch_archived": 0,
                      "fetch_announcements": 0,
                      "category_id": category,
                      "a": "fetch_topics",
                      "aops_logged_in": "false",
                      "aops_user_id": user_id,
                      "aops_session_id": session_id
                      }

        try:
            navi_page = curl(sub_url, c, post=postfields)
            parsed = json.loads(navi_page)

            resp = parsed['response']

            if 'no_more_topics' in resp and resp['no_more_topics']:
                break

            for post in resp['topics']:
                last_post_time = int(post['last_post_time'])
                fetch_after = last_post_time
                yield (category, post, None)
        except Exception as e:
            yield (None, None, e)

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
    post_txt = replace_canonical_tex(post_txt)
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

def crawl_category_pages(category, newest, oldest, extra_opt):
    c = get_curl()

    succ_posts = 0
    for category, post, e in list_category_links(category, newest, oldest, c):
        if e is not None:
            print_err("category %d" % category)
            break
        try:
            post_id = post['topic_id']
            sub_url = '/community/c{}h{}'.format(category, post_id)
            url = root_url + sub_url
            post_txt = crawl_post_page(sub_url, get_curl())
            process_post(post_id, post_txt, url)
        except (KeyboardInterrupt, SystemExit):
            print('[abort]')
            return 'abort'
        except BaseException as e:
            print_err("post %s (%s)" % (url, str(e)))
            continue

        # count on success
        succ_posts += 1

        # sleep to avoid over-frequent request.
        time.sleep(0.6)

    # log crawled page number
    page_log = open("page.log", "a")
    page_log.write('category %s: %d posts successful.\n' %
                   (category, succ_posts))
    page_log.close()
    return 'finish'

def help(arg0):
    print('DESCRIPTION: crawler script for artofproblemsolving.com.' \
          '\n\n' \
          'SYNOPSIS:\n' \
          '%s [-n | --newest <days>] ' \
          '[-o | --oldest <days>] ' \
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
            argv, "n:o:c:p:h", [
                'newest=',
                'oldest=',
                'category=',
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
    category = -1
    post = -1
    newest = 0
    oldest = 0

    for opt, arg in opts:
        if opt in ("-n", "--newest"):
            newest = int(arg)
            continue
        if opt in ("-o", "--oldest"):
            oldest = int(arg)
            continue
        if opt in ("-c", "--category"):
            category = int(arg)
            continue
        elif opt in ("-p", "--post"):
            post = int(arg)
            continue
        elif opt in ("--no-overwrite"):
            extra_opt["overwrite"] = False
        elif opt in ("--patrol"):
            extra_opt["patrol"] = True
        elif opt in ("--hook-script"):
            extra_opt["hookscript"] = arg
        else:
            help(args[0])

    if post > 0:
        sub_url = "/community/c{}h{}".format(category, post)
        full_url = root_url + sub_url
        post_txt = crawl_post_page(sub_url, get_curl())
        process_post(arg, post_txt, full_url)
        exit(0)

    if (category > 0):
        while True:
            # crawling newest pages
            r = crawl_category_pages(category, newest, oldest,
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
