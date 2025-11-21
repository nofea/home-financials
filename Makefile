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

.PHONY: all configure build clean reconfigure test valgrind memcheck help

all: build

help:
	@echo "Available targets:"
	@echo "  all          - Build the project (default)."
	@echo "  configure    - Configure the project using CMake."
	@echo "  build        - Build the project."
	@echo "  test         - Run tests."
	@echo "  valgrind     - Run Valgrind on test binaries."
	@echo "  memcheck     - Alias for valgrind."
	@echo "  reconfigure  - Force a re-configuration."
	@echo "  clean        - Clean build artifacts."
	@echo "  help         - Show this help message."

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

# Valgrind / memory-check target
# Usage: make valgrind
VALGRIND ?= valgrind
VALGRIND_FLAGS ?= --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1

valgrind: build
	@command -v $(VALGRIND) >/dev/null 2>&1 || { echo "valgrind not found. Install valgrind to run memory checks."; exit 1; }
	@echo "Running Valgrind on test binaries in '$(BUILD_DIR)/bin'..."
	@cd $(BUILD_DIR)/bin && \
	if ls test_* 1> /dev/null 2>&1; then \
		for tb in test_*; do \
			if [ -x "$$tb" ]; then \
				echo "=== Valgrind: $$tb ==="; \
				$(VALGRIND) $(VALGRIND_FLAGS) "./$$tb" || { echo "Valgrind detected errors in $$tb"; exit 1; }; \
			fi; \
		done; \
		echo "Valgrind checks passed."; \
	else \
		echo "No test binaries (test_*) found in '$(BUILD_DIR)/bin'. Build the tests first (make test)."; \
		exit 1; \
	fi

# alias
memcheck: valgrind

reconfigure:
	@echo "Forcing reconfigure: removing '$(BUILD_DIR)' and re-configuring..."
	@rm -rf $(BUILD_DIR)
	$(MAKE) configure

clean:
	@echo "Cleaning build artifacts: removing '$(BUILD_DIR)'..."
	@rm -rf $(BUILD_DIR)
