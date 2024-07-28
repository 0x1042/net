CC=/usr/bin/clang
CXX=/usr/bin/clang++
Generates=Ninja
VCPKG_PATH=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake
OUTPUT=target
TARGET=server

config:
	cmake -B ${OUTPUT} -G Ninja -S . -DCMAKE_TOOLCHAIN_FILE=${VCPKG_PATH}

build: config
	cmake --build ${OUTPUT}

clean:
	rm -fr ${OUTPUT}
	rm -fr bazel-*

run: build
	./${OUTPUT}/${TARGET}

lint:
	clang-tidy --format-style=file --header-filter=. -p ${OUTPUT} .

lib:
	bazel build //lib:proxy

server:
	bazel build //:sserver