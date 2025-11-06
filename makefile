CC := gcc
CFLAGS := -std=c23

# Allow for additional flags from the command line
ifeq ($(USER_CFLAGS),)
  # If USER_CFLAGS is not set, use the base CFLAGS
else
  # If USER_CFLAGS is set, append it to CFLAGS
  CFLAGS += $(USER_CFLAGS)
endif

LDFLAGS := 
LDLIBS := 

# Thanks to Job Vranish (https://spin.atomicobject.com/2016/08/26/makefile-c-projects/)
TARGET_EXEC := rsa64.exe

BUILD_DIR := ./build
BUILD_DIRW := build
SRC_DIRS := .
SRC_DIRSW :=
INC_DIRS := 
MAKEFILE := makefile

# Find all the C files we want to compile
SRCS := $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.c))

# Prepends BUILD_DIR and appends .o to every src file
# As an example, ./hello.c turns into ./build/hello.o
OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)

# String substitution (suffix version without %).
# As an example, ./build/hello.o turns into ./build/hello.d
DEPS := $(OBJS:.o=.d)

# Add a prefix to INC_DIRS. So moduleA would become -ImoduleA. GCC understands this -I flag
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

# The -MMD and -MP flags together generate Makefiles for us!
# These files will have .d instead of .o as the output.
CPPFLAGS := $(INC_FLAGS) -MMD -MP

# The final build step.
$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS) $(MAKEFILE)
	$(CC) $(OBJS) -o $@ $(LDFLAGS) $(LDLIBS)

# Build step for C source
$(BUILD_DIR)/%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

.PHONY: dir
dir:
	if not exist $(BUILD_DIRW) mkdir $(BUILD_DIRW)
	$(foreach dir,$(SRC_DIRSW),if not exist $(BUILD_DIRW)\$(dir) mkdir $(BUILD_DIRW)\$(dir))

.PHONY: clean
clean:
	@echo $(SRCS)
	@echo $(OBJS)

	del $(BUILD_DIRW)\*
	$(foreach dir,$(SRC_DIRSW),del $(BUILD_DIRW)\$(dir)\* && rmdir $(BUILD_DIRW)\$(dir))
	rmdir $(BUILD_DIRW)

-include $(DEPS)
