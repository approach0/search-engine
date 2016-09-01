AROBJS := fork/linenoise.o
include ../rules.mk

all: lib

lib: $(BUILD_DIR)/liblinenoise.a

$(BUILD_DIR)/liblinenoise.a: $(AROBJS)
	$(MAKE) -C fork CC=gcc
	$(CREAT_BUILD_DIR)
	mv fork/*.o $(BUILD_DIR)
	$(COLOR_AR)
	$(strip $(GEN_LIB))

clean:
	$(MAKE) -C fork clean
	@ echo 'clean'
