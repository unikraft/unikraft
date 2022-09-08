#!/usr/bin/make -f

ARGTABLE_SRC = $(wildcard *.c)
ARGTABLE_OBJ = $(sort $(ARGTABLE_SRC:.c=.o))

ARGTABLE_LIB = libargtable3.a

$(ARGTABLE_OBJ): %.o: %.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(ARGTABLE_LIB): $(ARGTABLE_OBJ)
	$(AR) rc $@ $^
	$(RANLIB) $@

build: all

all: $(ARGTABLE_LIB)

install:

clean:
	rm -f $(ARGTABLE_OBJ) $(ARGTABLE_LIB)
