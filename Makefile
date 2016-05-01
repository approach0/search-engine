.PHONY: frame_all
include rules.mk

# a phony target is used here before we can get dependencies from frame.mk 
all: frame_all tags

# print colorful notice for make entering
MODULE_ENTER_PRINT =  @ tput setaf 1 && echo '[$@] ' && tput sgr0;

# general rules are listed below 

# project library files
%-module:
	$(MODULE_ENTER_PRINT)
	$(MAKE) -C $* lib

# specify modules to be included for building
TARGETS_FILE = targets.mk

# list the module names you want to compile
MODULE_NAMES := $(shell cat ${TARGETS_FILE})

# expand variables after we get MODULE_NAMES
MODULE_NAMES := $(MODULE_NAMES:=-module)
MODULE_BINS := $(MODULE_NAMES:=-bin)
MODULE_CLEAN := $(MODULE_NAMES:=-clean)

# force some dependencies in dep.mk
-include dep.mk

# now we can list all prerequisites
frame_all: $(MODULE_BINS)

# binary files are dependent on library files (second pass)
%-module-bin: $(MODULE_NAMES)
	$(MODULE_ENTER_PRINT)
	$(MAKE) -C $* all

# clean rules
%-module-clean:
	$(MODULE_ENTER_PRINT)
	$(MAKE) -C $* clean

clean: tags-clean $(MODULE_CLEAN)

# ctag related files
# (ctag is used for source code view using text editor like Vim)
SRC_LIST_FILE := srclist.txt
SRC_LIST := $(shell test -e ${SRC_LIST_FILE} && \
         cat ${SRC_LIST_FILE})

tags: $(SRC_LIST)
	$(HIGHLIGHT_BEGIN) 5
	@ echo '[create source list file]'
	$(HIGHLIGHT_END)
	$(FIND) \( -name '*.[hcly]' -o \
	-name '*.cpp' \) -a -print > $(SRC_LIST_FILE)
	$(HIGHLIGHT_BEGIN) 5
	@ echo '[create ctags]'
	$(HIGHLIGHT_END)
	@ if command -v ctags; then \
		ctags --langmap=c:.c.y -L $(SRC_LIST_FILE); \
	else \
		echo 'no ctags command.'; \
	fi

tags-clean:
	$(HIGHLIGHT_BEGIN) 5
	@echo '[clean ctags]'
	$(HIGHLIGHT_END)
	@$(FIND) -type f \( -name 'tags' \) -print | xargs rm -f
	$(HIGHLIGHT_BEGIN) 5
	@echo '[remove source list file]'
	$(HIGHLIGHT_END)
	@rm -f $(SRC_LIST_FILE)
