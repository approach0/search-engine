include ../config.mk
include ../rules.mk
include ../module.mk

.PHONY: dist

LOCAL_DIST := lib
MODULE_NAME := pya0
PY_CONFIG=python3-config

CFLAGS += `$(PY_CONFIG) --cflags`
LDFLAGS += --shared `$(PY_CONFIG) --ldflags`

all: dist

dist: .build/$(MODULE_NAME).so
	@ tput setaf 5 && echo '[binding] $^' && tput sgr0
	@ mkdir -p $(LOCAL_DIST)
	mv $< $(LOCAL_DIST)/

.build/$(MODULE_NAME).so: $(wildcard $(BUILD_DIR)/*.py.o)
	$(COLOR_LINK)
	gcc $^ $(LDOBJS) -Xlinker "-(" $(LDLIBS) -Xlinker "-)" $(LDFLAGS) -o $@

clean:
	rm -rf $(LOCAL_DIST)
	rm -rf build dist *.egg-info
	@ echo 'clean'
