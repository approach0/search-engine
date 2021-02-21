include ../config.mk
include ../rules.mk
include ../module.mk

CFLAGS += `python3-config --cflags`
LDFLAGS += --shared `python3-config --ldflags`

all: build/texlex.so

build/texlex.so: $(BUILD_DIR)/texlex.py.o
	mkdir -p ./build
	$(COLOR_LINK)
	$(LINK)

clean:
	@ echo 'clean'
