CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -Wpedantic -Wshadow -Isrc

HEADERS = src/error.h src/result.h
TESTS = test_error test_result

.PHONY: all test clean docs

all: macros $(TESTS)

macros: macros.c $(HEADERS)
	$(CC) $(CFLAGS) macros.c -o macros

test_error: tests/test_error.c src/error.h
	$(CC) $(CFLAGS) tests/test_error.c -o test_error

test_result: tests/test_result.c $(HEADERS)
	$(CC) $(CFLAGS) tests/test_result.c -o test_result

test: $(TESTS) macros
	./test_error
	./test_result
	./macros

docs:
	doxygen Doxyfile

clean:
	rm -f macros $(TESTS)
