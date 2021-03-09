import os
import json
import tempfile
import subprocess
import pya0
from .index_manager import get_cache_home


def _topic_process__ntcir12_math_browsing(line):
    fields = line.split()
    query = [{'type': 'tex', 'keyword': ' '.join(fields[1:])}]
    qid = fields[0]
    return qid, query


def _topic_process__ntcir12_math_browsing_concrete(line):
    fields = line.split()
    query = [{'type': 'tex', 'keyword': ' '.join(fields[1:])}]
    qid = fields[0]
    return qid, query


def _topic_process__ntcir12_math_browsing_wildcards(line):
    fields = line.split()
    query = [{'type': 'tex', 'keyword': ' '.join(fields[1:])}]
    qid = fields[0]
    return qid, query


def _topic_process__arqmath_2020_task1(json_item):
    query = [{'type': kw['type'], 'keyword': kw['str']} for kw in json_item['kw']]
    qid = 'A.' + str(json_item['qid'])
    return qid, query


def gen_topics_queries(collection: str):
    func_name = '_topic_process__' + collection.replace('-', '_')
    handler = globals()[func_name] if func_name in globals() else None
    cache = get_cache_home()
    curdir = os.path.dirname(os.path.abspath(__file__))
    prefix = f'{curdir}/topics-and-qrels/topics.{collection}'
    print(f'Searching topics file at: ${prefix} ...')
    found = False
    for src in [f'{prefix}.{ent}' for ent in ['txt', 'json']]:
        if not os.path.exists(src):
            continue
        else:
            found = True
        ext = src.split('.')[-1]
        if ext == 'txt':
            with open(src, 'r') as fh:
                for line in fh:
                    line = line.rstrip()
                    yield handler(line)
        elif ext == 'json':
            with open(src, 'r') as fh:
                qlist = json.load(fh)
                for json_item in qlist:
                    yield handler(json_item)
    if not found:
        raise ValueError(f'Unrecognized index name {collection}')


def get_qrels_filepath(collection: str):
    curdir = os.path.dirname(os.path.abspath(__file__))
    path = f'{curdir}/topics-and-qrels/qrels.{collection}.txt'
    if os.path.exists(path):
        return path
    else:
        return None


def trec_eval(qrels: str, run: str, eval_args: str):
    extra_args = eval_args.split() if eval_args else []
    cmd = ['/usr/local/bin/trec_eval', qrels, run, *extra_args]
    print(f'Invoking trec_eval: {cmd}', end='')
    try:
        process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = process.communicate()
        print(stderr.decode("utf-8"), end='')
        return stdout.decode("utf-8")
    except:
        print('\n\n\t Please install trec_eval: https://github.com/approach0/trec_eval', end="\n\n")
        quit(1)


def run_topics(index, collection, output, topk=1000, verbose=False, trec_eval_args=[]):

    # preparing evaluation (determine temp file, trec_output etc.)
    if os.path.exists(output):
        print(f'Truncating TREC output file {output}...')
        open(output, 'w').close() # truncate the file

    tmpout = tempfile.mktemp(".trec")

    # generate run file from specifed topics
    for qid, query in gen_topics_queries(collection):
        print('[ query topic ]', qid, query)

        # actually run query
        print(index, query, verbose, topk, tmpout)

        result_JSON = pya0.search(index, query,
            verbose=verbose,
            topk=topk,
            trec_output=tmpout
            # TREC line format: _QRY_ID_ docID url rank score runID
        )
        results = json.loads(result_JSON)

        ret_code = results['ret_code']
        ret_msg = results['ret_str']
        n_hits = len(results['hits']) if ret_code == 0 else 0

        RED = '\033[31m'
        RST = '\033[0m'
        if n_hits == 0: print(RED, end='')
        print(f'[ result ] {ret_msg}(#{ret_code}): {n_hits} hit(s)')
        if n_hits == 0: print(RST, end='')

        # inject query ID and append temp file to trec_output file
        with open(tmpout, 'r') as temp_fh:
            temp_contents = temp_fh.read()
            temp_contents = temp_contents.replace('_QRY_ID_', qid)
            with open(output, 'a') as output_fh:
                output_fh.write(temp_contents)

    # now invoke trec_eval ...
    qrels = get_qrels_filepath(collection)
    eval_output = trec_eval(qrels, output, trec_eval_args)
    print('\n --- trec_eval output ---\n' + eval_output, end='')
