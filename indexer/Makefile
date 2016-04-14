include ../rules.mk

OTHER_OBJS := lex.txt.o
include ../module.mk

lex.txt.c: lexer.txt.l
	$(LEX)

clean:
	rm -f lex.txt.c
	@ echo 'clean'
