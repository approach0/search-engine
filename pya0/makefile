include ../config.mk
include ../rules.mk
include ../module.mk

.PHONY: dist

SUB_MODULES := lexer
LOCAL_DIST := lib
MODULE_NAME := pya0
PY_CONFIG=python3-config

CFLAGS += `$(PY_CONFIG) --cflags`
LDFLAGS += --shared `$(PY_CONFIG) --ldflags`
OBJS := $(addsuffix .py.o, $(addprefix $(BUILD_DIR)/, $(SUB_MODULES)))

all: dist

dist: .build/$(MODULE_NAME).so
	@ tput setaf 5 && echo '[binding] $^' && tput sgr0
	@ mkdir -p $(LOCAL_DIST)
	mv $< $(LOCAL_DIST)/

.build/$(MODULE_NAME).so: $(OBJS)
	@ mkdir -p $(BUILD_DIR)
	$(COLOR_LINK)
	$(LINK)

clean:
	rm -rf $(LOCAL_DIST)
	rm -rf build dist *.egg-info
	@ echo 'clean'
