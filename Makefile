BUILD_DIR   ?= build
BUILD_TYPE  ?= Release
PREFIX      ?= /usr

.PHONY: all clean install uninstall package dist

all: $(BUILD_DIR)/build.ninja
	cmake --build $(BUILD_DIR) -- -j$(shell nproc)

$(BUILD_DIR)/build.ninja:
	cmake -B $(BUILD_DIR) -G Ninja \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DCMAKE_INSTALL_PREFIX=$(PREFIX)

clean:
	rm -rf $(BUILD_DIR)

install: all
	sudo cmake --install $(BUILD_DIR)

uninstall:
	sudo cmake --build $(BUILD_DIR) --target uninstall 2>/dev/null || \
	sudo xargs rm -f < $(BUILD_DIR)/install_manifest.txt

package: all
	cd $(BUILD_DIR) && cpack

dist: $(BUILD_DIR)/build.ninja
	cd $(BUILD_DIR) && cpack --config CPackSourceConfig.cmake
