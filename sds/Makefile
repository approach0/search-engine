ARCHIVE = $(BUILD_DIR)/lib$(CURDIRNAME).a
ALL_OBJS += $(BUILD_DIR)/sds.o

include ../rules.mk
include ../module.mk

$(BUILD_DIR)/%.o: $(wildcard fork/*.c)
	$(COLOR_CC)
	$(CREAT_BUILD_DIR)
	$(CC_DEP) $@ $(CFLAGS) $^ > $(BUILD_DIR)/$*.d
	$(CC) -c $(CFLAGS) $(filter %.c, $^) -o $@

sds.h: fork/sds.h
	ln -sf $< $@

clean:
	$(MAKE) -C fork clean
	@ echo 'clean'
