CC=/usr/bin/clang
CXX=/usr/bin/clang++
Generates=Ninja
# VCPKG_PATH=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake
OUTPUT=target
TARGET=server

config:
	# cmake -B ${OUTPUT} -G Ninja -S . -DCMAKE_TOOLCHAIN_FILE=${VCPKG_PATH}
	cmake -B ${OUTPUT} -G Ninja -S .

build: config
	cmake --build ${OUTPUT}

lint:
	clang-tidy --format-style=file --header-filter=. -p ${OUTPUT} .

# https://mmoemulator.com/p/going-super-sonic-with-asio/

clean:
	cmake --build ${OUTPUT} --target clean


