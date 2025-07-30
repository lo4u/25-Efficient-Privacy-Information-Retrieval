FROM ubuntu:22.04 AS builder

# 可以传入的参数值
# 这些ARG变量在构建过程中可以被 --build-arg 覆盖
ARG NOAVX512="OFF" # 默认不禁用AVX-512
ARG NOAVX2="OFF"   # 默认不禁用AVX2
ARG NOCRT="OFF"    # 默认不禁用CRT

RUN apt-get update && \
		apt-get install -y build-essential clang cmake git libtool libssl-dev python3 \
		libomp-dev pkg-config curl zip unzip tar \
RUN apt install -y libboost-all-dev && \
		rm -rf /var/lib/apt/lists/*

WORKDIR /app

RUN mkdir 25-Efficient-Privacy-Information-Retrieval
COPY 25-Efficient-Privacy-Information-Retrieval /app/25-Efficient-Privacy-Information-Retrieval

ARG PROJECT_DIR=/app/25-Efficient-Privacy-Information-Retrieval
WORKDIR ${PROJECT_DIR}
# 安装hexl
ARG VCPKG_DIR=${PROJECT_DIR}/vcpkg
RUN git clone https://github.com/microsoft/vcpkg.git

WORKDIR ${VCPKG_DIR}
RUN ./bootstrap-vcpkg.sh -disbleMetrics
ENV VCPKG_ROOT=${VCPKG_DIR}
ENV PATH=${VCPKG_DIR}:${PATH}

RUN ./vcpkg install hexl

WORKDIR ${PROJECT_DIR}

RUN cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake \
			-DNOAVX512=${NOAVX512} -DNOAVX2=${NOAVX2}	-DNOCRT=${NOCRT} \
RUN cmake --build build -j4 -- PARAMSET=PARAMS_DYNAMIC \
  		TEXP=8 TEXPRIGHT=56 TCONV=4 TGSW=8 QPBITS=20 PVALUE=256 \
  		QNUMFIRST=1 QNUMREST=0 OUTN=2


FROM ubuntu:22.04
WORKDIR /app
COPY --from=builder /app/25-Efficient-Privacy-Information-Retrieval/build /app/pir/build
WORKDIR /app/pir/build/bin