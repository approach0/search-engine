include ../rules.mk

OTHER_OBJS := lex.mix.o lex.eng.o
include ../module.mk

all: jieba-wrap.py

jieba-wrap.py: ../txt-seg/jieba-wrap.py
	ln -sf $<

lex.mix.c: lexer.mix.l
	$(LEX)

lex.eng.c: lexer.eng.l
	$(LEX)

clean:
	rm -f lex.*.c
	@ echo 'clean'
