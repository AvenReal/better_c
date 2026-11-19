CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -Wpedantic -Wshadow

HEADERS = error.h result.h better_c.h

.PHONY: all test clean

all: macros

macros: macros.c $(HEADERS)
	$(CC) $(CFLAGS) macros.c -o macros

test_better_c: test_better_c.c $(HEADERS)
	$(CC) $(CFLAGS) test_better_c.c -o test_better_c

test: test_better_c macros
	./test_better_c
	./macros

docs:
	doxygen Doxyfile

clean:
	rm -f macros test_better_c
