.PHONY: all clean test expecttest unittest fmt fmt-ci debug

BUILD_DIR = build
SRC_DIR = src
TESTS_DIR = test
TARGET = $(BUILD_DIR)/main

all: $(TARGET)

CFLAGS := -std=gnu17 -Wall -Wextra -Werror -O2

debug: CFLAGS += -g
debug: $(TARGET)

SOURCES := $(wildcard $(SRC_DIR)/*.c)

$(TARGET): $(SOURCES) | $(BUILD_DIR)
	gcc $(CFLAGS) -o $@ $^

TESTTARGET = $(BUILD_DIR)/test_runner
TESTS := $(wildcard $(TESTS_DIR)/*.c) $(filter-out src/main.c,$(SOURCES))
CHECK_CFLAGS := $(shell pkg-config --cflags check)
CHECK_LIBS   := $(shell pkg-config --libs check)

$(TESTTARGET): $(TESTS) | $(BUILD_DIR)
	gcc $(CFLAGS) $(CHECK_CFLAGS) -g -o $@ $^ $(CHECK_LIBS)

test: unittest expecttest

unittest: $(TESTTARGET)
	./$(TESTTARGET)

EXPECTTESTS := $(wildcard $(TESTS_DIR)/test_*.exp)

expecttest: debug
	@echo "Running all expect tests..."
	@for t in $(EXPECTTESTS); do \
		echo "Running $$t..."; \
		expect $$t || { echo "Test $$t FAILED"; exit 1; }; \
	done
	@echo "All tests passed!"

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

fmt:
	clang-format -i $(shell find $(SRC_DIR) $(TESTS_DIR) -name "*.c" -o -name "*.h")

fmt-ci:
	clang-format --dry-run -Werror -i $(shell find $(SRC_DIR) $(TESTS_DIR) -name "*.c" -o -name "*.h")

clean:
	rm -rf $(BUILD_DIR)
