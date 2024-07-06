CC=/usr/bin/clang
CXX=/usr/bin/clang++
Generates=Ninja
VCPKG_PATH=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake

TARGET=server

config:
	cmake -B _build -G Ninja -S . -DCMAKE_TOOLCHAIN_FILE=${VCPKG_PATH}

build: config
	cmake --build _build

clean:
	rm -fr _build

run: build
	./_build/${TARGET}