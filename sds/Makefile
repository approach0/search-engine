include ../rules.mk
include ../module.mk

.PHONY: objectfiles

all: $(BUILD_DIR)/libsds.a sds.h

$(BUILD_DIR)/libsds.a: objectfiles

objectfiles: $(wildcard fork/*.c)
	$(CC) -c $(CFLAGS) $(filter %.c, $^)

sds.h: fork/sds.h
	ln -sf $< $@

clean:
	$(MAKE) -C fork clean
	@ echo 'clean'
