CC=/usr/bin/clang
CXX=/usr/bin/clang++
Generates=Ninja
VCPKG_PATH=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake

TARGET=server

config:
	cmake -B target -G Ninja -S . -DCMAKE_TOOLCHAIN_FILE=${VCPKG_PATH}

build: config
	cmake --build target

clean:
	rm -fr target

run: build
	./target/${TARGET}
