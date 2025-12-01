FROM ubuntu:22.04 AS builder

# 可以传入的参数值
# 这些ARG变量在构建过程中可以被 --build-arg 覆盖
ARG NOAVX512="OFF" # 默认不禁用AVX-512
ARG NOAVX2="OFF"   # 默认不禁用AVX2
ARG NOCRT="OFF"    # 默认不禁用CRT

# 换清华源（可选）
RUN sed -i 's|http://.*archive.ubuntu.com|http://mirrors.tuna.tsinghua.edu.cn|g' /etc/apt/sources.list && \
    sed -i 's|http://.*security.ubuntu.com|http://mirrors.tuna.tsinghua.edu.cn|g' /etc/apt/sources.list && \
    apt-get update
RUN apt update && \
		apt install -y libboost-all-dev build-essential clang cmake libssl-dev libtool && \
		apt install -y git python3 pkg-config curl zip unzip tar && \
		rm -rf /var/lib/apt/lists/*	

RUN git clone https://github.com/llvm/llvm-project.git --depth=1 --filter=blob:none && \
    mkdir -p llvm-project/openmp/build
WORKDIR llvm-project/openmp/build

RUN cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DLIBOMP_ENABLE_SHARED=OFF \
  -DLIBOMP_ENABLE_STATIC=ON \
  -DCMAKE_INSTALL_PREFIX=/usr/local
RUN make -j && make install

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

RUN cmake -S . -B build -DSTATICMODE=ON \
			-DNOAVX512=${NOAVX512} -DNOAVX2=${NOAVX2}	-DNOCRT=${NOCRT}
RUN cmake --build build -j4 -- PARAMSET=PARAMS_DYNAMIC \
  		TEXP=8 TEXPRIGHT=56 TCONV=4 TGSW=8 QPBITS=20 PVALUE=256 \
  		QNUMFIRST=1 QNUMREST=0 OUTN=2


FROM ubuntu:22.04
WORKDIR /app
COPY --from=builder /app/25-Efficient-Privacy-Information-Retrieval/spiral /app/pir/
WORKDIR /app/pir/