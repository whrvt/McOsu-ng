FROM registry.gitlab.steamos.cloud/proton/sniper/sdk:latest AS main-deps

ENV DEBIAN_FRONTEND=noninteractive \
    PATH="/usr/lib/gcc-14/bin:$PATH"

RUN update-alternatives --install /usr/bin/gcc gcc /usr/lib/gcc-14/bin/gcc 90 \
    --slave /usr/bin/g++ g++ /usr/lib/gcc-14/bin/g++ \
    --slave /usr/bin/gcov gcov /usr/lib/gcc-14/bin/gcov

RUN apt-get -y update && \
    apt-get -y install pkgconf autoconf build-essential \
    libx11-dev libxi-dev libglu1-mesa-dev libjpeg-dev libfreetype-dev \
    zlib1g-dev nasm cmake meson ccache unzip zip patchelf libltdl-dev && \
    rm -rf /var/lib/apt/lists/*

FROM main-deps AS manual-deps

WORKDIR /tempbuild

ENV GENIE_VERSION="bf3bf3f051a5f829e9f77d792c169ff866528f8d"
# needed for SoLoud build
RUN wget -O genie.tar.gz "https://github.com/bkaradzic/GENie/archive/${GENIE_VERSION}.tar.gz" && \
    tar -xf genie.tar.gz && \
    cd GENie-${GENIE_VERSION} && \
    git init && git config user.email "temp@temp.temp" && git config user.name "temp" && git commit --allow-empty -m "temp" && \
    make && \
    ./bin/linux/genie release && \
    make -C build/gmake.linux clean && \
    make -C build/gmake.linux all && \
    install -D -m755 $PWD/bin/linux/genie /usr/bin/genie

FROM manual-deps AS main-build

COPY .github/workflows/docker/build.sh /build.sh
RUN chmod +x /build.sh

WORKDIR /src
ENTRYPOINT ["/build.sh"]
