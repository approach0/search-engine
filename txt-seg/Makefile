OTHER_OBJS := lex.mix.o lex.eng.o

include ../config.mk
include ../rules.mk
include ../module.mk

CFLAGS += -I $(JIEBA)/include -I $(JIEBA)/deps

all:

lex.mix.c: lexer.mix.l
	$(COLOR_LEX)
	$(strip $(DO_LEX))

lex.eng.c: lexer.eng.l
	$(COLOR_LEX)
	$(strip $(DO_LEX))

clean:
	rm -f lex.*.c
	@ echo 'clean'
