include ../rules.mk

OTHER_OBJS := lex.txt.o
include ../module.mk

all: jieba-wrap.py

jieba-wrap.py: ../txt-seg/jieba-wrap.py
	ln -sf $<

lex.txt.c: lexer.txt.l
	$(LEX)

clean:
	rm -f lex.txt.c
	@ echo 'clean'
