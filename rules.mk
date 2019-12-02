.PHONY: all clean regular-clean new

# .SECONDARY with no prerequisite prevents intermediate
# objects (here *.main.o) from being deleted by GNU-make.
.SECONDARY:

# build files (e.g. .o/.d) directory
BUILD_DIR := .build

# current directory name
CURDIRNAME := $(notdir $(CURDIR))

# executables (e.g. .out) directory
RUN_DIR := run

# create build directory
CREAT_BUILD_DIR := @ mkdir -p $(BUILD_DIR)

# reset CFLAGS
CFLAGS = -Wall -Wno-unused-function -Wno-unused-result \
	-fno-builtin -fno-strict-aliasing -Wno-format-truncation \
	-D_DEFAULT_SOURCE -O2
# (_DEFAULT_SOURCE enables strdup function and DT_* macro)

# target color print
TARGET_COLOR_PRINT =  @ tput setaf 1 && echo '[$@] ' && tput sgr0;

# preprocessor print
PP := python3 ../proj-preprocessor.py
COLOR_PP =  @ tput setaf 5 && echo "[preprocess] $<" && tput sgr0

# compiler
CC := gcc -std=gnu99
CC_DEP := @ gcc -MM -MT
COLOR_CC =  @ tput setaf 5 && echo "[compile C source] $<" && \
       tput sgr0
COMPILE_CC = $(CC) -c $(CFLAGS) $(filter %.c, $^) -o $@

CXX := g++
CXX_DEP = @ g++ -MM -MT
COLOR_CXX = @ tput setaf 5 && echo '[compile C++ source] $<' && \
       tput sgr0
COMPILE_CXX = $(CXX) -c $(CFLAGS) $(filter %.cpp, $^) -o $@

SHOW_OBJ_UNDEF_SYMBOLS = nm --undefined-only $@
CCDH = gcc -dH

# linker
MHOOK_FLAGS := -Wl,--wrap,malloc -Wl,--wrap,free \
-Wl,--wrap,realloc -Wl,--wrap,calloc -Wl,--wrap,strdup
LD := gcc $(MHOOK_FLAGS)
COLOR_LINK = @ tput setaf 5 && echo '[link] $@' && tput sgr0

# cycling libraries even if we use "-( .. -)", this is necessary
# to fix Ubuntu gcc linking problem.
LINK = $(LD) $^ $(LDOBJS) -Xlinker "-(" $(LDLIBS) -Xlinker "-)" \
	$(LDFLAGS) -Xlinker "-(" $(LDLIBS) -Xlinker "-)" -o $@

# archive
AR := ar
COLOR_AR = @ tput setaf 5 && echo '[archive] $@' && tput sgr0
EXTRACT_DEP_LIBS = $(foreach arlib, $(ARLIBS), ar -x ${arlib};)
MV_DEP_OBJS := find . -maxdepth 1 -name '*.o' -type f -print0 \
	-exec mv {} $(BUILD_DIR) \; > /dev/null
GEN_LIB = rm -f $@ && $(AR) rcs $@ $(BUILD_DIR)/*.o
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

# set Position-independent code, so that linker can relocate
# symbols for shared library.
CFLAGS += -fPIC

# handle debug flag here, e.g., `make all debug=0'
# (you need to rebuild all project objects when debug flag is changed.
# Also notice that -D gets defined before the source file is parsed)
debug ?= 0 # debug=0 by default
ifeq ($(debug), 1)
    CFLAGS += -g
else
    CFLAGS += -DNDEBUG
endif

# regular rules
all:
	$(HIGHLIGHT_BEGIN) 5
	@echo "[done] $(CURDIR)"
	$(HIGHLIGHT_END)

new: clean all
	$(HIGHLIGHT_BEGIN) 5
	@echo "[re-make done] $(CURDIR)"
	$(HIGHLIGHT_END)

-include $(wildcard $(BUILD_DIR)/*.d)

$(BUILD_DIR)/%.o: %.c
	$(COLOR_CC)
	$(CREAT_BUILD_DIR)
	$(CC_DEP) $@ $(CFLAGS) $< > $(BUILD_DIR)/$*.d
	$(strip $(COMPILE_CC))

$(BUILD_DIR)/%.o: $(BUILD_DIR)/%.c
	$(COLOR_CC)
	$(CC_DEP) $@ $(CFLAGS) $^ > $(BUILD_DIR)/$*.d
	$(strip $(COMPILE_CC))

$(BUILD_DIR)/%.c: %.dt.c
	$(COLOR_PP)
	$(CREAT_BUILD_DIR)
	$(PP) $(INC_PATHS) $<  > $@

$(BUILD_DIR)/%.main.o: $(RUN_DIR)/%.c
	$(COLOR_CC)
	$(CREAT_BUILD_DIR)
	$(CC_DEP) $@ $(CFLAGS) $^ > $(BUILD_DIR)/$*.d
	$(strip $(COMPILE_CC))

$(BUILD_DIR)/%.main.o: $(BUILD_DIR)/%.main.c
	$(COLOR_CC)
	$(CC_DEP) $@ $(CFLAGS) $^ > $(BUILD_DIR)/$*.d
	$(strip $(COMPILE_CC))

$(BUILD_DIR)/%.main.c: $(RUN_DIR)/%.dt.c
	$(COLOR_PP)
	$(CREAT_BUILD_DIR)
	$(PP) $(INC_PATHS) $<  > $@

$(BUILD_DIR)/%.o: %.cpp
	$(COLOR_CXX)
	$(CREAT_BUILD_DIR)
	$(CXX_DEP) $@ $(CFLAGS) $^ > $(BUILD_DIR)/$*.d
	$(strip $(COMPILE_CXX))
	$(SHOW_OBJ_UNDEF_SYMBOLS) # to show C++ object symbols

$(RUN_DIR)/%.out: $(BUILD_DIR)/%.main.o
	$(COLOR_LINK)
	$(strip $(LINK))

$(BUILD_DIR)/lib%.a: $(ALL_OBJS)
	$(COLOR_AR)
	$(CREAT_BUILD_DIR)
	$(EXTRACT_DEP_LIBS)
	$(MV_DEP_OBJS)
	$(strip $(GEN_LIB))
	$(COLOR_SHOW_LIB)
	$(SHOW_LIB)

# header validity
validate-%.h: %.h
	$(CCDH) $(CFLAGS) $*.h

# library check
check-lib%:
	@ $(CXX) $(LDFLAGS) -w -Xlinker \
	--unresolved-symbols=ignore-all -l$* -o /dev/null

FIND := @ find . -type d \( -path './.git' -o -path '*/tmp' \) -prune -o

regular-clean:
	@ tput setaf 5 && echo [regular clean] && tput sgr0
	$(FIND) -type f \( -name '*.[dao]' \) -print | xargs rm -f
	$(FIND) -type f \( -name '*.output' \) -print | xargs rm -f
	$(FIND) -type f \( -name '*.tmp' \) -print | xargs rm -f
	$(FIND) -type f \( -name '*.gch' \) -print | xargs rm -f
	$(FIND) -type f \( -name '*.pyc' \) -print | xargs rm -f
	$(FIND) -type f \( -name '*.out' \) -print | xargs rm -f
	$(FIND) -type f \( -name '*.bin' \) -print | xargs rm -f
	$(FIND) -type l \( -name '*.ln' \) -print | xargs rm -f
	$(FIND) -type f \( -name '*.so' \) -print | xargs rm -f
	$(FIND) -type l \( -name '*.py' \) -print | xargs rm -f
	$(FIND) -type d \( -name 'tmp' \) -print | xargs rm -rf
	$(FIND) -type d \( -name '__pycache__' \) -print | xargs rm -rf
	$(FIND) -type d \( -name '$(BUILD_DIR)' \) -print | xargs rm -rf

grep-%:
	$(FIND) -type f \( -name '*.[ch]' \)   -exec grep -C 2 --color -nH $* {} \;
	$(FIND) -type f \( -name '*.cpp' \)    -exec grep -C 2 --color -nH $* {} \;
	$(FIND) -type f \( -name '*.[ly]' \)   -exec grep -C 2 --color -nH $* {} \;
	$(FIND) -type f \( -name 'Makefile' \) -exec grep -C 2 --color -nH $* {} \;
	$(FIND) -type f \( -name '*.mk' \)     -exec grep -C 2 --color -nH $* {} \;

show-swp:
	$(FIND) -type f \( -name '*.swp' \) -print

clean: regular-clean
