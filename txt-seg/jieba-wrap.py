#encoding=utf-8
from jieba.posseg import POSTokenizer
import sys
import jieba

posseg_tok = None

def jieba_wrap_init():
	global posseg_tok
	posseg_tok = POSTokenizer(jieba.dt)

def jieba_wrap_cut(utf8_txt):
	result = jieba.dt.tokenize(utf8_txt, mode="search")
	for tok in result:
		tok_str   = tok[0]
		pos_begin = tok[1]
		pos_end   = tok[2]
		tok_tag   = posseg_tok.word_tag_tab.get(tok_str, 'x')
		if tok_tag == 'x':
			# in this case it can be M(number), Eng(English) or X(unknown)
			# so we need to further investigate and update tok_tag.
			res = posseg_tok._POSTokenizer__cut_detail(tok_str)
			_, tok_tag = next(res)
		yield (tok_str, pos_begin, pos_end, tok_tag)

def jieba_wrap_print_gen(generator):
	for a,b,c,d in generator:
		print("%s\t start: %d \t end:%d \t tag:%s" % (a,b,c,d))

def jieba_wrap_add(utf8_term):
	jieba.add_word(utf8_term)

if __name__ == "__main__":
	utf8_str = "其实，BB机的流行是早在1995年以前的事情了：之后有了Internet。"
	jieba_wrap_init()
	print("=== example 1 ===")
	gen = jieba_wrap_cut("未来石墨烯材料")
	jieba_wrap_print_gen(gen)
	print('after adding user word...')
	jieba_wrap_add('石墨烯');
	gen = jieba_wrap_cut("未来石墨烯材料")
	jieba_wrap_print_gen(gen)
	print("=== example 2 ===")
	gen = jieba_wrap_cut(utf8_str)
	jieba_wrap_print_gen(gen)
