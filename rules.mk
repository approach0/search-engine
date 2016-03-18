.PHONY: all lib clean regular-clean

# compiler
CFLAGS = -Wall -Wno-unused-function
CC =  @ tput setaf 5 && echo -n '[compile C source] ' && \
       tput sgr0 && echo $< && gcc
CXX = @ tput setaf 5 && echo -n '[compile C++ source] ' && \
       tput sgr0 && echo $< && g++
CC_DEP = @ gcc
CXX_DEP = @ g++

# linker
LD = @ tput setaf 5 && echo -n '[link $(LDLIBS) $(LDOBJS)] ' \
     && tput sgr0 && echo $@ && gcc

LINK = $(LD) $(LDFLAGS) $^ $(LDOBJS) \
	-Xlinker "-(" $(LDLIBS) -Xlinker "-)" -o $@

# archive
AR = @ tput setaf 5 && echo -n '[archiev $^] ' \
     && tput sgr0 && echo $@ && ar rcs $@ $^

# Bison/Flex
LEX = @ tput setaf 5 && echo -n '[lex] ' \
        && tput sgr0 && echo $< && flex $<

YACC = @ tput setaf 5 && echo -n '[yacc] ' \
         && tput sgr0 && echo $< \
         && bison -v -d --report=itemset $< -o y.tab.c

# regular rules
all: 
	@echo "[done $(CURDIR)]"

include $(wildcard *.d)

%.o: %.c
	$(CC) $(CFLAGS) $*.c -c -o $@
	$(CC_DEP) -MM -MT $@ $(CFLAGS) $*.c -o $*.d

%.o: %.cpp
	$(CXX) $(CFLAGS) $*.cpp -c -o $@
	$(CXX_DEP) -MM -MT $@ $(CFLAGS) $*.cpp -o $*.d

%.out: %.o
	$(LINK)

FIND := @ find . -type d \( -path './.git' \) -prune -o
regular-clean:
	@ tput setaf 5 && echo [regular clean] && tput sgr0
	$(FIND) -type f \( -name '*.[dao]' \) -print | xargs rm -f
	$(FIND) -type f \( -name '*.output' \) -print | xargs rm -f
	$(FIND) -type f \( -name '*.tmp' \) -print | xargs rm -f
	$(FIND) -type f \( -name '*.gch' \) -print | xargs rm -f
	$(FIND) -type f \( -name '*.pyc' \) -print | xargs rm -f
	$(FIND) -type f \( -name '*.out' \) -print | xargs rm -f
	$(FIND) -type f \( -name '*.log' \) -print | xargs rm -f
	$(FIND) -type l \( -name '*.ln' \) -print | xargs rm -f
	$(FIND) -type f \( -name '*.so' \) -print | xargs rm -f

clean: regular-clean
