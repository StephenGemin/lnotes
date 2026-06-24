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

.PHONY: all build test test-scripts test-raise install uninstall clean

all: build

build: $(BUILD_DIR)/obl

$(BUILD_DIR)/obl: $(SOURCES_MAIN) $(HEADERS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc -o $@ $(SOURCES_MAIN) $(LDFLAGS)

test: $(BUILD_DIR)/test_obl
	$(BUILD_DIR)/test_obl

$(BUILD_DIR)/test_obl: $(SOURCES_TEST) $(HEADERS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc -o $@ $(SOURCES_TEST) $(LDFLAGS)

test-scripts: build
	@bash tests/test_scripts.sh

test-raise: build
	@bash tests/test_raise_command.sh

install: build
	@PREFIX=$(PREFIX) bash scripts/install.sh

uninstall:
	@PREFIX=$(PREFIX) bash scripts/uninstall.sh $(if $(FORCE),-f)

clean:
	rm -rf $(BUILD_DIR)
