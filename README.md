# Efficient-Implementation-of-Privacy-Information-Retrieval
This is an efficient implementation of Privacy Information Retrieval (PIR) with Spiral, PBC and OCC/OCD.

# 编译
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake
<!-- cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake -->
cmake --build build -j4 -- PARAMSET=PARAMS_DYNAMIC \
  TEXP=8 TEXPRIGHT=56 TCONV=4 TGSW=8 QPBITS=20 PVALUE=256 \
  QNUMFIRST=1 QNUMREST=0 OUTN=2


# 调试
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=/home/lo4u/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build -j4 -- PARAMSET=PARAMS_DYNAMIC TEXP=4 TEXPRIGHT=56 TCONV=4 TGSW=4 QPBITS=14 PVALUE=4 QNUMFIRST=1 QNUMREST=0 OUTN=2 ADDTL_CXX_FLAGS=

# 执行
./spiral --dim0 6 --dim1 4 --database database.txt --output result.txt --query 0 10000 20000 30000 40000