include ../config.mk
include ../rules.mk
include ../module.mk

CFLAGS += `python-config --cflags`
LDFLAGS += --shared `python-config --ldflags`

all: build/texlex.so

build/texlex.so: $(BUILD_DIR)/texlex.py.o
	mkdir -p ./build
	$(COLOR_LINK)
	$(LINK)

clean:
	@ echo 'clean'
