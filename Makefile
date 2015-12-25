include rules.mk

MODULES := hello hello2 list da-trie txt-seg wstring
EXT_MODULES := cjieba

MODULES := $(MODULES:=-module)
EXT_MODULES := $(EXT_MODULES:=-module)

EXT_MODULES_LIBS := $(EXT_MODULES:=-extlib)
MODULES_LIBS := $(MODULES:=-lib)
MODULES_BINS := $(MODULES:=-bin)
MODULES_CLEAN := $(MODULES:=-clean) $(EXT_MODULES:=-clean)

all: tags $(MODULES_BINS)

%-module-lib:
	make -C $* lib

%-module-extlib:
	make -C $*

%-module-bin: $(MODULES_LIBS) $(EXT_MODULES_LIBS)
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
