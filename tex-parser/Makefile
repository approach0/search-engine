include ../rules.mk

# these objects are generated at compile-time.
OTHER_OBJS := lex.yy.o y.tab.o gen-trans.o
include ../module.mk

enum-symbol.h: gen-symbol.h 
	ln -s gen-symbol.h enum-symbol.h

gen-lexer.l: gen-symbol.h 
gen-symbol.h: lexer.l symbol.h.template
	./gen-symbol.sh $^ gen-lexer.l gen-symbol.h 

gen-trans.c: enum-symbol.h enum-token.h trans.c.template
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
