OBJ := ittnotify_static.o jitprofiling.o

LIBNAME := libittnotify.a

INCLUDE += -I ../../../include

CFLAGS += $(INCLUDE) -fPIC -Wno-strict-prototypes
CC ?= gcc
CC_VERSION := $(shell $(CC) -dumpversion)
CC_NO_LESS_THAN_8 := $(shell expr $(CC_VERSION) \>\= "8")
ifeq ($(CC_NO_LESS_THAN_8), 1)
    CFLAGS += -fcf-protection
endif

.PHONY: all
all: $(LIBNAME)

$(LIBNAME): $(OBJ)
	$(AR) rcs $@ $^

.PHONY: clean
clean:
	@$(RM) $(LIBNAME) $(OBJ)
