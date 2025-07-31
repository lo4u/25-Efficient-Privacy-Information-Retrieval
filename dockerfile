FROM ubuntu:22.04 AS builder

# 可以传入的参数值
# 这些ARG变量在构建过程中可以被 --build-arg 覆盖
ARG NOAVX512="OFF" # 默认不禁用AVX-512
ARG NOAVX2="OFF"   # 默认不禁用AVX2
ARG NOCRT="OFF"    # 默认不禁用CRT


# RUN	apt update && \
# 		apt install -y libboost-all-dev libomp-dev build-essential clang libtool libssl-dev && \
#   	apt install -y  git python3 pkg-config curl zip unzip wget tar sudo make && \
# 		rm -rf /var/lib/apt/lists/*

# # 通过源码安装高版本cmake
# RUN wget https://github.com/Kitware/CMake/releases/download/v3.31.8/cmake-3.31.8.tar.gz && \
# 		tar -xvf cmake-3.31.8.tar.gz && \
# 		cd cmake-3.31.8 && ./bootstrap && \
# 		make && sudo make install

RUN apt update && \
		apt install -y libboost-all-dev libomp-dev build-essential clang cmake libssl-dev libtool && \
		apt install -y git python3 pkg-config curl zip unzip tar && \
		rm -rf /var/lib/apt/lists/*	

WORKDIR /app

RUN git clone https://github.com/lo4u/25-Efficient-Privacy-Information-Retrieval.git

ARG PROJECT_DIR=/app/25-Efficient-Privacy-Information-Retrieval
WORKDIR ${PROJECT_DIR}
# 安装hexl
ARG VCPKG_DIR=${PROJECT_DIR}/vcpkg
RUN git clone https://github.com/microsoft/vcpkg.git

WORKDIR ${VCPKG_DIR}
RUN ./bootstrap-vcpkg.sh -disableMetrics
ENV VCPKG_ROOT=${VCPKG_DIR}
ENV PATH=${VCPKG_DIR}:${PATH}

RUN ./vcpkg install hexl

WORKDIR ${PROJECT_DIR}

RUN cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake \
			-DNOAVX512=${NOAVX512} -DNOAVX2=${NOAVX2}	-DNOCRT=${NOCRT}
RUN cmake --build build -j4 -- PARAMSET=PARAMS_DYNAMIC \
  		TEXP=8 TEXPRIGHT=56 TCONV=4 TGSW=8 QPBITS=20 PVALUE=256 \
  		QNUMFIRST=1 QNUMREST=0 OUTN=2


FROM ubuntu:22.04
WORKDIR /app
COPY --from=builder /app/25-Efficient-Privacy-Information-Retrieval/build /app/pir/build
COPY --from=builder /usr/lib/llvm-14/lib/libomp.so.5 /lib/x86_64-linux-gnu/libomp.so.5
WORKDIR /app/pir/build/bin