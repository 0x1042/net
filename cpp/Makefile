CC=/usr/bin/clang
# CC=/usr/local/opt/llvm/bin/clang
CXX=/usr/bin/clang++
# CXX=/usr/local/opt/llvm/bin/clang++
Generates=Ninja
VCPKG_PATH=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake
OUTPUT=target
TARGET=server

config:
	cmake -B ${OUTPUT} -G Ninja -S . -DCMAKE_TOOLCHAIN_FILE=${VCPKG_PATH} -DCMAKE_CXX_INCLUDE_WHAT_YOU_USE="include-what-you-use"

build: config
	cmake --build ${OUTPUT}

lint:
	clang-tidy --format-style=file --header-filter=. -p ${OUTPUT} .

# https://mmoemulator.com/p/going-super-sonic-with-asio/