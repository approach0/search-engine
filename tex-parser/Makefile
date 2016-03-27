include ../rules.mk

# objects below are generated at compile-time.
OTHER_OBJS := lex.yy.o y.tab.o gen-trans.o
# source file below is excluded from compiling.
EXCLUDE_SRC := trans.template.c

include ../module.mk

gen-symbol.h: gen-lexer.l
gen-token.h: gen-lexer.l
gen-lexer.l: lexer.template.l symbol.template.h token.template.h
	./auto-gen.sh $^

gen-trans.c: gen-symbol.h gen-token.h trans.template.c
	./trans-gen.sh $^ $@

lex.yy.c: gen-lexer.l y.tab.h
	$(LEX)

y.tab.h y.tab.c: grammar.y
	$(YACC)

clean:
	rm -f *.tab.[ch]
	rm -f lex.yy.c
	rm -f gen-*
	rm -f *.dot
	@ echo 'clean'
