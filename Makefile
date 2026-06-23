BUILD_DIR ?= build
PREFIX    ?= $(HOME)/.local
CC        ?= gcc
CFLAGS    ?= -Wall -Wextra -Wpedantic -Werror -std=c99
LDFLAGS   ?=

# Add POSIX feature flags on non-Apple Unix
UNAME_S := $(shell uname -s)
ifneq ($(UNAME_S),Darwin)
  CFLAGS += -D_POSIX_C_SOURCE=200809L
endif

SOURCES_MAIN = src/main.c src/utils.c src/notes.c src/search.c
SOURCES_TEST = tests/test_main.c src/utils.c src/notes.c
HEADERS = src/notes.h

.PHONY: all build test install clean

all: build

build: $(BUILD_DIR)/notes

$(BUILD_DIR)/notes: $(SOURCES_MAIN) $(HEADERS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc -o $@ $(SOURCES_MAIN) $(LDFLAGS)

test: $(BUILD_DIR)/test_notes
	$(BUILD_DIR)/test_notes

$(BUILD_DIR)/test_notes: $(SOURCES_TEST) $(HEADERS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc -o $@ $(SOURCES_TEST) $(LDFLAGS)

install: build
	mkdir -p $(PREFIX)/bin
	cp $(BUILD_DIR)/notes $(PREFIX)/bin/notes

clean:
	rm -rf $(BUILD_DIR)
