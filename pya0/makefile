include ../config.mk
include ../rules.mk
include ../module.mk

.PHONY: dist

MODULE_NAME := pya0
PY_CONFIG = python3-config
CFLAGS += `$(PY_CONFIG) --cflags`

# Note that libpythonX.Y.so.1 is not on the list of libraries that a manylinux1
# extension is allowed to link to.
# See https://www.python.org/dev/peps/pep-0513/#libpythonx-y-so-1
#
#LDFLAGS += --shared `$(PY_CONFIG) --ldflags`

LOCAL_OBJS := $(wildcard ./*.py.c)
LOCAL_OBJS := $(addprefix ./$(BUILD_DIR)/, $(LOCAL_OBJS:.c=.o))

all: dist

dist: .build/$(MODULE_NAME).so
	@ tput setaf 5 && echo '[packaging] $^' && tput sgr0
	@ mkdir -p $(MODULE_NAME)
	@ mkdir -p $(MODULE_NAME)/topics-and-qrels
	rm -f $(MODULE_NAME)/*.py
	cp ./utils/*.py $(MODULE_NAME)
	rm -f $(MODULE_NAME)/topics-and-qrels/*
	cp ./topics-and-qrels/* $(MODULE_NAME)/topics-and-qrels/
	mv $< $(MODULE_NAME)/$(MODULE_NAME).so

.build/$(MODULE_NAME).so: $(LOCAL_OBJS)
	$(COLOR_LINK)
	gcc -shared $^ $(LDOBJS) -Xlinker "-(" $(LDLIBS) -Xlinker "-)" $(LDFLAGS) -o $@

clean:
	rm -rf $(MODULE_NAME)
	rm -rf build dist *.egg-info wheelhouse
	@ echo 'clean'
