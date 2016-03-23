# source files can have mixed code of C and CPP
SRC := $(wildcard *.c) $(wildcard *.cpp)

SRC_OBJS := $(SRC)
SRC_OBJS := $(SRC_OBJS:.c=.o)
SRC_OBJS := $(SRC_OBJS:.cpp=.o)

# test files / final executable files
RUN_SRC := $(wildcard run/*.c)
RUN_OBJS := $(RUN_SRC:.c=.o)
RUN_BINS := $(RUN_OBJS:.o=.out)

# when you run gcc ./path/main.c with a "path", you need 
# to specify current include directory.
CFLAGS = -I .
# but alternatively:
# a good practice is to write #include "module/module.h" 
# instead of just #include "module.h", so that another 
# module who uses this will also find the header file.
CFLAGS += -I ..

# union set of all objects (make all the elements unique),
# OTHER_OBJS is for objects files generated from something
# like Bison/Flex.
ALL_OBJS := $(sort $(SRC_OBJS) $(OTHER_OBJS))

# use object files in this module to link RUN_BINS
LDOBJS := $(ALL_OBJS)

# include dependency .mk files, e.g. dep-LDLIB.mk.
DEP_LINKS := $(wildcard dep-*.mk)
-include $(DEP_LINKS)

# strip off suffix ".mk" from DEP_LINKS where "LDFLAGS" can be found.
LDLIBS := $(foreach dep_link, ${DEP_LINKS}, \
              $(if $(findstring LDFLAGS, $(shell cat $(dep_link))), \
                  ${dep_link:.mk=} \
              ) \
          )

# further strip off leading "dep-" and append "-l" in the front.
LDLIBS := $(foreach dep_link, ${LDLIBS}, ${dep_link:dep-%=-l%})

# remove any trailing slash, e.g. "path/" to "path" in LDFLAGS
LDFLAGS := $(shell echo $(LDFLAGS)) # don't know why this is necessary :(
LDFLAGS := $(LDFLAGS:/=)

# define function to list dependency library paths (only list those exist).
# example input: -L ../hello -L ../world
# output: ../hello/libhello.a ../world/libworld.a
lib_paths = $(foreach lpath, $(filter-out -L, $(1:%/=%)), \
               $(wildcard ${lpath}/lib$(subst ../,,${lpath}).a) \
             )

# make an archive only if SRC_OBJS is non-empty.
ifneq ($(ALL_OBJS), )
CURDIRNAME := $(notdir $(CURDIR))
ARCHIVE := lib$(CURDIRNAME).a
MERGE_AR = $(call lib_paths, $(LDFLAGS))
$(ARCHIVE): $(ALL_OBJS)
	$(AR)
endif

# summary what a module needs to make
module_lib := $(ALL_OBJS) $(ARCHIVE)
module_bin := $(RUN_OBJS) $(RUN_BINS)
