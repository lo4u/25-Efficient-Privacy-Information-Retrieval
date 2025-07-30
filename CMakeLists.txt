cmake_minimum_required(VERSION 3.12)
project(spiral VERSION 0.1 LANGUAGES CXX)

set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)


# CXX options
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-g -O3 -std=c++17 -MD -march=native -fopenmp")

option(NOAVX512 "Disable AVX-512" OFF)
option(NOAVX2 "Disable AVX2" OFF)
if (NOAVX512)
    message("Disabling AVX-512")
    add_compile_options(-mno-avx512f)
endif()
if (NOAVX2)
    message("Disabling AVX2")
    add_compile_options(-mno-avx2)
endif()
if (NOCRT)
    message("Disabling CRT")
    add_compile_options(-DNOCRT=1)
endif()

# main target
# 设置可执行文件的输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")
set(SRC_FILES src/spiral.cpp src/core.cpp src/constants.cpp src/poly.cpp src/util.cpp src/client.cpp src/testing.cpp src/PBC/test.cpp src/PBC/cuckoo.cpp src/PBC/mod.cpp src/Multi-PIR/pir.cpp src/Multi-PIR/test.cpp src/Multi-PIR/multipir.cpp src/OCC/occ.cpp src/OCC/test.cpp src/OCC/rbmatrices.cpp src/OCD/okvs.cpp src/OCD/test.cpp src/OCD/3H_GCT.cpp src/benchmark/test.cpp src/Batch-PIR/main.cpp)
set(HEADER_FILES include/spiral.h include/core.h include/constants.h include/poly.h include/util.h include/client.h include/testing.h include/cuckoo.h include/mod.h include/pir.h include/multipir.h include/occ.h include/rbmatrices.h include/okvs.h include/3H_GCT.h)
add_executable(spiral ${SRC_FILES} ${HEADER_FILES})
target_include_directories(spiral PUBLIC include)
target_include_directories(spiral PUBLIC /home/$ENV{USER}/vcpkg/installed/)

# Must specify:
# -DCMAKE_TOOLCHAIN_FILE=/home/ubuntu/vcpkg/scripts/buildsystems/vcpkg.cmake

find_package(HEXL CONFIG REQUIRED)
target_link_libraries(spiral PRIVATE HEXL::hexl)

find_package(OpenSSL REQUIRED)
target_link_libraries(spiral PRIVATE OpenSSL::SSL OpenSSL::Crypto)

add_compile_definitions(
    TEXP=$(TEXP)
    TEXPRIGHT=$(TEXPRIGHT)
    TCONV=$(TCONV)
    TGSW=$(TGSW)
    QPBITS=$(QPBITS)
    PVALUE=$(PVALUE)
    QNUMFIRST=$(QNUMFIRST)
    QNUMREST=$(QNUMREST)
    OUTN=$(OUTN)
)