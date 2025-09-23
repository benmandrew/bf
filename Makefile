.PHONY: all clean test fmt fmt-ci debug

CFLAGS := -std=c17 -Wall -Wextra -Werror -pedantic -O2

BUILD_DIR = build
SRC_DIR = src
TESTS_DIR = test
TARGET = $(BUILD_DIR)/main

SOURCES := $(wildcard $(SRC_DIR)/*.c)
TESTS := $(wildcard $(TESTS_DIR)/test_*.exp)

all: $(TARGET)

debug: CFLAGS += -g
debug: $(TARGET)

$(TARGET): $(SOURCES) | $(BUILD_DIR)
	gcc $(CFLAGS) -o $@ $^

test: $(TARGET)
	@echo "Running all expect tests..."
	@for t in $(TESTS); do \
		echo "Running $$t..."; \
		expect $$t || { echo "Test $$t FAILED"; exit 1; }; \
	done
	@echo "All tests passed!"

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

fmt:
	clang-format -i $(shell find $(SRC_DIR) -name "*.c" -o -name "*.h")

fmt-ci:
	clang-format --dry-run -Werror -i $(shell find $(SRC_DIR) -name "*.c" -o -name "*.h")

clean:
	rm -f $(TARGET)
