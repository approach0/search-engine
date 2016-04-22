.PHONY: all clean regular-clean

# compiler
CFLAGS = -Wall -Wno-unused-function
CC =  @ tput setaf 5 && echo -n "[compile C source] " && \
       tput sgr0 && echo $< && gcc
CXX = @ tput setaf 5 && echo -n '[compile C++ source] ' && \
       tput sgr0 && echo $< && g++
CCDH =  @ tput setaf 5 && echo -n '[test C header] ' && \
       tput sgr0 && echo $< && gcc
CC_DEP = @ gcc
CXX_DEP = @ g++

# linker
LD = @ tput setaf 5 && echo -n '[link $(strip $*.o $(LDOBJS) $(LDLIBS))] ' \
     && tput sgr0 && echo $@ && gcc

LINK = $(LD) $(LDFLAGS) $*.o $(LDOBJS) \
	-Xlinker "-(" $(LDLIBS) -Xlinker "-)" -o $@

# archive
AR = @ tput setaf 5 && echo -n '[archive $(strip $(AROBJS) $(ARLIBS))] ' \
     && tput sgr0 && echo $@ && \
	 rm -f $@ && \
	 ar -rcT $@ $(AROBJS) $(ARLIBS) && ar -t $@ | tr '\n' ' ' && echo ''

# Bison/Flex
LEX = @ tput setaf 5 && echo -n '[lex] ' \
        && tput sgr0 && echo $< && flex $<

YACC = @ tput setaf 5 && echo -n '[yacc] ' \
         && tput sgr0 && echo $< \
		 && tput setaf 3 \
         && bison -v -d --report=itemset $< -o y.tab.c \
		 && tput sgr0;

# regular rules
all: 
	@echo "[done $(CURDIR)]"

-include $(wildcard *.d)
-include $(wildcard run/*.d)

%.o: %.c
	$(CC) $(CFLAGS) $*.c -c -o $@
	$(CC_DEP) -MM -MT $@ $(CFLAGS) $*.c -o $*.d

%.o: %.cpp
	$(CXX) $(CFLAGS) $*.cpp -c -o $@
	$(CXX_DEP) -MM -MT $@ $(CFLAGS) $*.cpp -o $*.d

%.out: %.o
	$(LINK)

test-%.h: %.h
	$(CCDH) $(CFLAGS) -dH $*.h

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

clean: regular-clean
