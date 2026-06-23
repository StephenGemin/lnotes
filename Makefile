BUILD_DIR ?= build

.PHONY: all build test install clean

all: build

build:
	cmake -B $(BUILD_DIR)
	cmake --build $(BUILD_DIR) --parallel

test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure

install: build
	cmake --install $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
