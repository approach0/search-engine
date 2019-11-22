include ../config.mk
include ../rules.mk

# objects below are associated to generated auto-generated source code.
OTHER_OBJS := lex.yy.o y.tab.o gen-trans.o gen-completion.o
# source file below is excluded from compiling.
EXCLUDE_SRC := trans.template.c completion.template.c

include ../module.mk

gen-symbol.h: gen-lexer.l
gen-token.h: gen-lexer.l
gen-lexer.l: lexer.template.l symbol.template.h token.template.h
	$(HIGHLIGHT_BEGIN) 5
	@ echo "generating $@"
	$(HIGHLIGHT_END)
	./auto-gen.sh $^

gen-trans.c: gen-symbol.h gen-token.h trans.template.c
	$(HIGHLIGHT_BEGIN) 5
	@ echo "generating $@"
	$(HIGHLIGHT_END)
	./trans-gen.sh $^ $@

gen-completion.c: lexer.template.l completion.template.c
	$(HIGHLIGHT_BEGIN) 5
	@ echo "generating $@"
	$(HIGHLIGHT_END)
	./completion-gen.sh $^ $@

lex.yy.c: gen-lexer.l y.tab.h
	$(COLOR_LEX)
	$(strip $(DO_LEX))

y.tab.h y.tab.c: grammar.y
	$(COLOR_YACC)
	@ echo '$(strip $(DO_YACC))'
	$(HIGHLIGHT_BEGIN) 3 # highlight conflicts
	@ $(DO_YACC)
	$(HIGHLIGHT_END)

clean:
	rm -f *.tab.[ch]
	rm -f lex.yy.c
	rm -f gen-*
	rm -f *.dot
	@ echo 'clean'
