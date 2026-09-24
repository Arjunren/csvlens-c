CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror
CPPFLAGS ?= -Iinclude
BUILD := build

.PHONY: all test clean
all: $(BUILD)/csvlens

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/csvlens: src/main.c src/csvlens.c include/csvlens.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) src/main.c src/csvlens.c -o $@

$(BUILD)/csvlens_tests: tests/test_csv.c src/csvlens.c include/csvlens.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_csv.c src/csvlens.c -o $@

test: $(BUILD)/csvlens_tests
	./$(BUILD)/csvlens_tests

clean:
	rm -rf $(BUILD)
