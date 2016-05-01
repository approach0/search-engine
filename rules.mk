.PHONY: all clean regular-clean new

# compiler
CFLAGS = -Wall -Wno-unused-function

CC := gcc -c
CC_DEP := @ gcc -MM -MT
COLOR_CC =  @ tput setaf 5 && echo "[compile C source] $<" && \
       tput sgr0
COMPILE_CC = $(CC) $(CFLAGS) $*.c -o $@

CXX := g++ -c
CXX_DEP = @ g++ -MM -MT
COLOR_CXX = @ tput setaf 5 && echo '[compile C++ source] $<' && \
       tput sgr0
COMPILE_CXX = $(CXX) $(CFLAGS) $*.cpp -o $@

CCDH = gcc -dH

# linker
LD := gcc # try `g++' if does not find stdc++ library.
COLOR_LINK = @ tput setaf 5 && echo '[link] $@' && tput sgr0

LINK = $(LD) $(LDFLAGS) -Xlinker "-(" \
	$(LDLIBS) $(LDOBJS) $*.o \
	-Xlinker "-)" -o $@
# archive
AR := ar
COLOR_AR = @ tput setaf 5 && echo '[archive] $@' && tput sgr0

GEN_LIB = rm -f $@ && $(AR) -rcT $@ $(AROBJS) $(ARLIBS)
COLOR_SHOW_LIB := @ tput setaf 5 && \
	echo 'objects in this archive:' && tput sgr0
SHOW_LIB = @ $(AR) -t $@ | tr '\n' ' ' && echo ''

# make
MAKE := make --no-print-directory

# Bison/Flex
LEX := flex
COLOR_LEX = @ tput setaf 5 && echo '[lex] $<' && tput sgr0
DO_LEX = flex $<

YACC := bison
COLOR_YACC = @ tput setaf 5 && echo '[yacc] $<' && tput sgr0
DO_YACC = $(YACC) -v -d --report=itemset $< -o y.tab.c

# high light shorhand
HIGHLIGHT_BEGIN := @ tput setaf
HIGHLIGHT_END := @ tput sgr0

# regular rules
all:
	$(HIGHLIGHT_BEGIN) 5
	@echo "[done] $(CURDIR)"
	$(HIGHLIGHT_END)

new: clean all
	$(HIGHLIGHT_BEGIN) 5
	@echo "[re-make done] $(CURDIR)"
	$(HIGHLIGHT_END)

-include $(wildcard *.d)
-include $(wildcard run/*.d)

%.o: %.c
	$(COLOR_CC)
	$(strip $(COMPILE_CC))
	$(CC_DEP) $@ $(CFLAGS) $*.c -o $*.d

%.o: %.cpp
	$(COLOR_CXX)
	$(strip $(COMPILE_CXX))
	$(CXX_DEP) $@ $(CFLAGS) $*.cpp -o $*.d

%.out: %.o
	$(COLOR_LINK)
	$(strip $(LINK))

test-%.h: %.h
	$(CCDH) $(CFLAGS) $*.h

FIND := @ find . -type d \( -path './.git' \) -prune -o

regular-clean:
	@ tput setaf 5 && echo [regular clean] && tput sgr0
	$(FIND) -type f \( -name '*.[dao]' \) -print | xargs rm -f
	$(FIND) -type f \( -name '*.output' \) -print | xargs rm -f
	$(FIND) -type f \( -name '*.tmp' \) -print | xargs rm -f
	$(FIND) -type f \( -name '*.gch' \) -print | xargs rm -f
	$(FIND) -type f \( -name '*.pyc' \) -print | xargs rm -f
	$(FIND) -type f \( -name '*.out' \) -print | xargs rm -f
	$(FIND) -type f \( -name '*.bin' \) -print | xargs rm -f
	$(FIND) -type f \( -name '*.log' \) -print | xargs rm -f
	$(FIND) -type l \( -name '*.ln' \) -print | xargs rm -f
	$(FIND) -type f \( -name '*.so' \) -print | xargs rm -f
	$(FIND) -type l \( -name '*.py' \) -print | xargs rm -f
	$(FIND) -type d \( -name 'tmp' \) -print | xargs rm -rf
	$(FIND) -type d \( -name '__pycache__' \) -print | xargs rm -rf

grep-%:
	$(FIND) -type f \( -name '*.[ch]' \)   -exec grep --color -nH $* {} \;
	$(FIND) -type f \( -name '*.cpp' \)    -exec grep --color -nH $* {} \;
	$(FIND) -type f \( -name '*.[ly]' \)   -exec grep --color -nH $* {} \;
	$(FIND) -type f \( -name 'Makefile' \) -exec grep --color -nH $* {} \;
	$(FIND) -type f \( -name '*.mk' \)     -exec grep --color -nH $* {} \;

show-swp:
	$(FIND) -type f \( -name '*.swp' \) -print

clean: regular-clean
