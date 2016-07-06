BUILD_DIR := ./build

all lib: build_dir
	(cd $(BUILD_DIR) && cmake ..)
	(cd $(BUILD_DIR) && make)

build_dir:
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
	@ echo 'clean'
