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

# add include paths to CFLAGS
INC_PATHS := -I . -I ..
CFLAGS += $(INC_PATHS)

# union set of all objects (make all the elements unique),
# OTHER_OBJS is for objects files generated from something
# like Bison/Flex.
OTHER_OBJS := $(addprefix $(BUILD_DIR)/, $(OTHER_OBJS))
ALL_OBJS += $(sort $(SRC_OBJS) $(OTHER_OBJS))

# include dependency .mk files, e.g. dep-LDLIB.mk.
DEP_LINKS := $(wildcard $(DEP_DIR)/dep-*.mk)
-include $(DEP_LINKS)

# strip off suffix ".mk" from DEP_LINKS where "LDFLAGS" can be found.
LDLIBS := ${DEP_LINKS:.mk=}

# further strip off leading "dep-" and replace with "-l".
LDLIBS := $(foreach dep_link, ${LDLIBS}, ${dep_link:$(DEP_DIR)/dep-%=-l%})

# get module paths of only internal dependency.
DEP_MODS := $(foreach path, $(LDFLAGS), \
                $(if $(findstring $(BUILD_DIR), ${path}), \
                     $(shell echo $(subst /$(BUILD_DIR),, ${path})) \
                 ) \
             )

# get library paths of only internal dependency.
DEP_LIBS := $(foreach depmod, $(subst ../,,$(DEP_MODS)), \
              ../${depmod}/$(BUILD_DIR)/lib${depmod}.a)

# link to local .a only if ALL_OBJS is non-empty.
ifneq ($(ALL_OBJS), )
# add local .a into LDLIBS for linking binaries.
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
ARLIBS = $(sort $(call lib_paths, $(LDFLAGS)))
endif

# add new .PHONY name
.PHONY: lib bin

# top rules.
all: lib bin
lib: $(DEP_LIBS) $(ALL_OBJS) $(ARCHIVE)
bin: $(RUN_BINS)

# more specific dependency rules ...
# re-create archive if any lib changes.
$(ARCHIVE): $(ALL_OBJS)

# re-create archive if any external lib updates.
$(ARCHIVE): $(DEP_LIBS)

# rebuild all bins if any lib changes.
$(RUN_BINS): $(DEP_LIBS) $(ALL_OBJS) $(ARCHIVE)

# project-wise dependency awareness
PROJ_DEP_MK := proj-dep.mk
-include ../$(PROJ_DEP_MK)

# include carry-alone external dependency libs
LDLIBS += $(${CURDIRNAME:%=%.carryalone-extdep}:%=-l%)
LDLIBS := $(sort ${LDLIBS})

# include carry-alone dependency paths (in dep-*.mk files)
CARRY_ALONG_DEP_MK := $(${CURDIRNAME:%=%.carryalone-intdep})
CARRY_ALONG_DEP_MK := ${CARRY_ALONG_DEP_MK:%=../%/${DEP_DIR}/dep-*.mk}
-include ${CARRY_ALONG_DEP_MK}

# update all direct/indirect dependencies
update: $(CURDIRNAME)-module
	$(MAKE) bin

# issue make in other modules
%-module:
	$(TARGET_COLOR_PRINT)
	$(MAKE) -C ../$* lib
