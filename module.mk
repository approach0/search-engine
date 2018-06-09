# dependency files (e.g. dep-*.mk) directory.
# other directories (i.e. BUILD_DIR and RUN_DIR) are defined in rules.mk
DEP_DIR := dep

# source files can have mixed code of C and CPP
SRC := $(wildcard *.c) $(wildcard *.cpp)

SRC_OBJS := $(SRC)
SRC_OBJS := $(filter-out $(EXCLUDE_SRC),$(SRC))
SRC_OBJS := $(SRC_OBJS:.dt.c=.o)
SRC_OBJS := $(SRC_OBJS:.c=.o)
SRC_OBJS := $(SRC_OBJS:.cpp=.o)
SRC_OBJS := $(addprefix $(BUILD_DIR)/, $(SRC_OBJS))

# test files / final executable files
RUN_SRC := $(wildcard $(RUN_DIR)/*.c)
RUN_SRC := $(filter-out $(EXCLUDE_SRC),$(RUN_SRC))
RUN_BINS := $(RUN_SRC:.dt.c=.out)
RUN_BINS := $(RUN_BINS:.c=.out)

INC_PATHS = -I . -I ..

CFLAGS += $(INC_PATHS)

# union set of all objects (make all the elements unique),
# OTHER_OBJS is for objects files generated from something
# like Bison/Flex.
OTHER_OBJS := $(addprefix $(BUILD_DIR)/, $(OTHER_OBJS))
ALL_OBJS := $(sort $(SRC_OBJS) $(OTHER_OBJS))

# include dependency .mk files, e.g. dep-LDLIB.mk.
DEP_LINKS := $(wildcard $(DEP_DIR)/dep-*.mk)
-include $(DEP_LINKS)

# strip off suffix ".mk" from DEP_LINKS where "LDFLAGS" can be found.
LDLIBS := $(foreach dep_link, ${DEP_LINKS}, \
              $(if $(findstring LDFLAGS, $(shell cat $(dep_link))), \
                  ${dep_link:.mk=} \
              ) \
          )

# further strip off leading "dep-" and replace with "-l".
LDLIBS := $(foreach dep_link, ${LDLIBS}, ${dep_link:$(DEP_DIR)/dep-%=-l%})

# link to local .a only if ALL_OBJS is non-empty.
ifneq ($(ALL_OBJS), )
# add local .a into LDLIBS for linking binaries.
CURDIRNAME := $(notdir $(CURDIR))
LDLIBS += -l$(CURDIRNAME)
LOCAL_LDPATH := -L$(BUILD_DIR)
LDFLAGS += $(LOCAL_LDPATH)
endif

# we use .a to link RUN_BINS, no .o files needed
LDOBJS :=

# remove any trailing slash, e.g. "path/" to "path" in LDFLAGS
LDFLAGS := $(shell echo $(LDFLAGS)) # don't know why this is necessary :(
LDFLAGS := $(LDFLAGS:/=)

# define function to list dependency libraries (only list those exist),
# and filter out LOCAL_LDPATH to avoid local AR libraries (./build/*.a).
# example input: -L ../<external>/build -L ../<local>/build
# output: -L ../<external>/build/*.a
lib_paths = $(foreach lpath, $(filter-out $(LOCAL_LDPATH), $(1:%/=%)), \
                $(wildcard ${lpath}/*.a) \
             )

# make an archive only if ALL_OBJS is non-empty.
ifneq ($(ALL_OBJS), )
ARCHIVE := $(BUILD_DIR)/lib$(CURDIRNAME).a
AUTO_MERGE_AR = $(call lib_paths, $(LDFLAGS))

# let ar know what objects/libraries to archive.
ARLIBS = $(sort $(AUTO_MERGE_AR) $(OTHER_MERGE_AR))
endif

# summary what a module needs to make.
module_lib := $(ALL_OBJS) $(ARCHIVE)
module_bin := $(RUN_BINS)

# list module conventional rules.
.PHONY: lib

all: $(module_lib) $(module_bin)
bin: run/test.out
lib: $(module_lib)

# rebuild all bins if any lib changes.
$(module_bin): $(module_lib)
