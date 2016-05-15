include ../rules.mk

OTHER_OBJS := lex.mix.o lex.eng.o
include ../module.mk

CFLAGS += $(shell python3-config --cflags)
LDFLAGS += $(shell python3-config --ldflags)

all: jieba-wrap.py

jieba-wrap.py: ../txt-seg/jieba-wrap.py
	ln -sf $<

lex.mix.c: lexer.mix.l
	$(COLOR_LEX)
	$(strip $(DO_LEX))

lex.eng.c: lexer.eng.l
	$(COLOR_LEX)
	$(strip $(DO_LEX))

clean:
	rm -f lex.*.c
	@ echo 'clean'
