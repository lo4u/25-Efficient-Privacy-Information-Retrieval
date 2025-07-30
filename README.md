# Efficient-Implementation-of-Privacy-Information-Retrieval
This is an efficient implementation of Privacy Information Retrieval (PIR) with Spiral, PBC and OCC/OCD.

# 编译
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake
<!-- cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake -->
cmake --build build -j4 -- PARAMSET=PARAMS_DYNAMIC \
  TEXP=8 TEXPRIGHT=56 TCONV=4 TGSW=8 QPBITS=20 PVALUE=256 \
  QNUMFIRST=1 QNUMREST=0 OUTN=2

# 执行
./spiral v1 v2 index...
比如："./spiral 3 3  23 45 12"，表示数据库设置为总共 2^3 * 2 * 2 * 2 = 64个元素，查询第23 45 12号元素。这里的index从1开始，命令行输出的索引从0开始。