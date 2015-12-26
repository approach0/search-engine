include ../rules.mk

OTHER_OBJS := lex.yy.o y.tab.o
include ../module.mk

all: $(module_lib) $(module_bin)

lib: $(module_lib)

enum-symbol.h gen-lexer.l: lexer.l symbol.h
	./gen-symbol-tab.sh $^
	rm -f enum-symbol.h
	ln -s gen-symbol.h enum-symbol.h

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
	@ echo 'clean'
