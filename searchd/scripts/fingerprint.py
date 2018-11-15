#!/usr/bin/python3
import requests
import json
import sys
import numpy as np
import matplotlib.pyplot as plt

labelA = 'ecir'
labelB = 'pruning'
urlA = "http://localhost:3838/get/ecir/%s/judged"
urlB = "http://localhost:3838/get/pruning/%s/judged"
FULLREL = 3
PARTREL = 1

def get_request(url):
	headers = {'Content-Type': 'application/json', 'Accept': 'application/json'}
	try:
		print(url)
		r = requests.get(url, headers=headers)
		j = json.loads(r.content.decode('utf-8'))
		# print(j)
		return j
	except Exception as e:
		print('Error:', e)
		sys.exit(1)

def get_restuple(j, rel):
	arr = []
	for (idx, i) in enumerate(j['res']):
		rank, judge_rel = int(i['rank']), int(float(i['judge_rel']))
		arr.append((idx, judge_rel >= rel))
	return arr

def qry_eventdat(url, qry, rel=0):
	j = get_request(url % qry)
	return [idx for (idx, hit) in get_restuple(j, rel) if hit]

def draw(qry_num, rel):
	qry = 'NTCIR12-MathWiki-' + str(qry_num)
	arrA = qry_eventdat(urlA, qry, rel)
	arrB = qry_eventdat(urlB, qry, rel)
	data = np.array([arrA, arrB])
	# draw event plot
	colors = np.array([[1, 0, 0], [0, 1, 0]])
	plt.xticks([0, 1], [labelA, labelB])
	plt.eventplot(data, orientation='vertical', colors=colors)
	savefile = './tmp/%s-rel%s.png' % (qry, rel)
	print(savefile)
	plt.savefig(savefile)
	plt.close()
	# plt.show()

### DEBUG ####
# draw(5, 3)
# quit()
##############

for rel in [FULLREL, PARTREL]:
	for num in [_ for _ in range(1, 21) if _ != 2]:
		draw(num, rel)
