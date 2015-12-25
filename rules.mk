.PHONY: all clean common-clean

# compiler
CFLAGS = -Wall -Wno-unused-function
CC = @ echo '[compile C source]' $< && gcc 
CXX = @ echo '[compile C++ source]' $< && g++
CC_DEP = @ gcc
CXX_DEP = @ g++

# linker
LD = @ echo '[link $(LDLIBS) $(LDOBJS)] $@' && gcc

LINK = $(LD) $(LDFLAGS) $^ $(LDOBJS) \
	-Xlinker "-(" $(LDLIBS) -Xlinker "-)" -o $@

# archive
AR = @ echo '[archiev $^]' $@ && ar rcs $@ $^

# Bison/Flex
LEX = @ echo '[lex]' $< && flex $<
YACC = @ echo '[yacc]' $< && bison -v -d --report=itemset -g $< -o y.tab.c

# common rules
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
common-clean:
	@ echo [common clean]
	$(FIND) -type f \( -name '*.[dao]' \) -print | xargs rm -f
	$(FIND) -type f \( -name '*.output' \) -print | xargs rm -f
	$(FIND) -type f \( -name '*.tmp' \) -print | xargs rm -f
	$(FIND) -type f \( -name '*.gch' \) -print | xargs rm -f
	$(FIND) -type f \( -name '*.pyc' \) -print | xargs rm -f
	$(FIND) -type f \( -name '*.out' \) -print | xargs rm -f
	$(FIND) -type f \( -name '*.log' \) -print | xargs rm -f
	$(FIND) -type l \( -name '*.ln' \) -print | xargs rm -f
	$(FIND) -type f \( -name '*.so' \) -print | xargs rm -f

clean: common-clean
