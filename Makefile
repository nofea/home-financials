##############################################################################
# Makefile — helpers for configuring and building with CMake
#
# Targets:
#   make configure   -> run cmake to configure the build in ./build
#   make build       -> configure (if needed) and build the project
#   make clean       -> remove temporary/build files (build directory)
##############################################################################

CMAKE ?= cmake
BUILD_DIR ?= build
CMAKE_BUILD_TYPE ?= Release
CMAKE_FLAGS ?= -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE)

.PHONY: all configure build clean reconfigure test

all: build

configure:
	@echo "Configuring project (build dir: '$(BUILD_DIR)')..."
	@mkdir -p $(BUILD_DIR)
	$(CMAKE) -S . -B $(BUILD_DIR) $(CMAKE_FLAGS)

build: configure
	@echo "Building project..."
	$(CMAKE) --build $(BUILD_DIR) --parallel

test: build
	@echo "Running tests (ctest) in '$(BUILD_DIR)'..."
	@cd $(BUILD_DIR) && \
	if [ -x "$(BUILD_DIR)/bin/test_storage_manager" ]; then \
		"$(BUILD_DIR)/bin/test_storage_manager" --gtest_output=xml:$(BUILD_DIR)/test_results.xml || (echo "Test binary failed"; exit 1); \
	else \
		ctest --output-on-failure -C $(CMAKE_BUILD_TYPE); \
	fi

reconfigure:
	@echo "Forcing reconfigure: removing '$(BUILD_DIR)' and re-configuring..."
	@rm -rf $(BUILD_DIR)
	$(MAKE) configure

clean:
	@echo "Cleaning build artifacts: removing '$(BUILD_DIR)'..."
	@rm -rf $(BUILD_DIR)
