include ../rules.mk

# objects below are generated at compile-time.
OTHER_OBJS := lex.yy.o y.tab.o gen-trans.o
# source file below is excluded from compiling.
EXCLUDE_SRC := trans.template.c

include ../module.mk

enum-symbol.h: gen-symbol.h 
	ln -s gen-symbol.h enum-symbol.h

gen-lexer.l: gen-symbol.h 
gen-symbol.h: lexer.l symbol.template.h
	./gen-symbol.sh $^ gen-lexer.l gen-symbol.h 

gen-trans.c: enum-symbol.h enum-token.h trans.template.c
	./gen-trans.sh $^ $@

lex.yy.c: gen-lexer.l y.tab.h
	$(LEX)

y.tab.h y.tab.c: grammar.y
	$(YACC)

clean:
	rm -f *.tab.[ch]
	rm -f lex.yy.c
	rm -f enum-symbol.h
	rm -f gen-*.l
	rm -f gen-*.h
	rm -f gen-*.c
	rm -f *.dot
	@ echo 'clean'
