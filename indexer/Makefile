include ../rules.mk

OTHER_OBJS := lex.mix.o
include ../module.mk

all:

lex.mix.c: lexer.mix.l
	$(COLOR_LEX)
	$(strip $(DO_LEX))

clean:
	rm -f lex.*.c
	@ echo 'clean'
