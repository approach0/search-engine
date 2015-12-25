# source files can have mixed code of C and CPP
SRC:=$(wildcard *.c) $(wildcard *.cpp)

SRC_OBJS:=$(SRC)
SRC_OBJS:=$(SRC_OBJS:.c=.o)
SRC_OBJS:=$(SRC_OBJS:.cpp=.o)

# test files / final executable files
RUN_SRC:=$(wildcard run/*.c)
RUN_OBJS:=$(RUN_SRC:.c=.o)
RUN_BINS:=$(RUN_OBJS:.o=.out)

CFLAGS := -I .

# make an archive only if SRC_OBJS is non-empty.
ifneq ($(SRC_OBJS), ) 
CURDIRNAME := $(notdir $(CURDIR))
ARCHIVE := lib$(CURDIRNAME).a
$(ARCHIVE): $(SRC_OBJS)
	$(AR)
endif

# use object files in this module to link RUN_BINS
LDOBJS := $(SRC_OBJS)

# include dependency .mk files, e.g. dep-LDLIB.mk.
DEP_LINKS := $(wildcard dep-*.mk)
-include $(DEP_LINKS)

# strip off suffix ".mk" from DEP_LINKS
LDLIBS := $(foreach dep_link, ${DEP_LINKS}, ${dep_link:.mk=})
# further strip off leading "dep"
LDLIBS := $(foreach dep_link, ${LDLIBS}, ${dep_link:dep%=%})

# summary what a module needs to make
module := $(SRC_OBJS) $(RUN_OBJS) $(ARCHIVE) $(RUN_BINS)
