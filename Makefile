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
SOURCES_TEST = tests/test_utils.c src/utils.c src/notes.c
HEADERS = src/notes.h

.PHONY: all build test install uninstall clean

all: build

build: $(BUILD_DIR)/obl

$(BUILD_DIR)/obl: $(SOURCES_MAIN) $(HEADERS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc -o $@ $(SOURCES_MAIN) $(LDFLAGS)

test: build $(BUILD_DIR)/test_utils
	$(BUILD_DIR)/test_utils
	@for f in tests/test_*.sh; do bash $$f || exit 1; done

$(BUILD_DIR)/test_utils: $(SOURCES_TEST) $(HEADERS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc -o $@ $(SOURCES_TEST) $(LDFLAGS)

install: build
	@PREFIX=$(PREFIX) bash scripts/install.sh

uninstall:
	@PREFIX=$(PREFIX) bash scripts/uninstall.sh $(if $(FORCE),-f)

clean:
	rm -rf $(BUILD_DIR)
