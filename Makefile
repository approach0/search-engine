include rules.mk

MODULES := hello hello2 list da-trie txt-seg
MODULES := $(MODULES:=-module)
MODULES_CLEAN := $(MODULES:=-clean)

all: tags $(MODULES)

%-module:
	make -C $*

%-module-clean:
	make -C $* clean

clean: tags-clean $(MODULES_CLEAN)

SRC_LIST_FILE := srclist.txt
SRC_LIST := $(shell test -e ${SRC_LIST_FILE} && \
         cat ${SRC_LIST_FILE})

tags: $(SRC_LIST)
	@ echo '[create ctags]'
	$(FIND) -name '*.[hcly]' -o \
	-name '*.cpp' -o -print > $(SRC_LIST_FILE)
	@ if command -v ctags; then \
		ctags --langmap=c:.c.y -L $(SRC_LIST_FILE); \
	else \
		echo 'no ctags command.'; \
	fi

tags-clean:
	@echo '[tags clean]'
	@$(FIND) -type f \( -name 'tags' \) -print | xargs rm -f
	@rm -f $(SRC_LIST_FILE)
