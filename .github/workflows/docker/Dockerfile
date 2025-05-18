FROM registry.gitlab.steamos.cloud/proton/sniper/sdk:3.0.20250210.116596-0

ENV DEBIAN_FRONTEND=noninteractive \
    PATH="/usr/lib/gcc-14/bin:$PATH"

RUN update-alternatives --install /usr/bin/gcc gcc /usr/lib/gcc-14/bin/gcc 90 \
    --slave /usr/bin/g++ g++ /usr/lib/gcc-14/bin/g++ \
    --slave /usr/bin/gcov gcov /usr/lib/gcc-14/bin/gcov

RUN apt-get -y update && \
    apt-get -y install pkgconf autoconf build-essential \
    libx11-dev libxi-dev libglu1-mesa-dev libjpeg-dev libfreetype-dev \
    zlib1g-dev nasm cmake meson ccache && \
    rm -rf /var/lib/apt/lists/*

COPY .github/workflows/docker/build.sh /build.sh
RUN chmod +x /build.sh

WORKDIR /src
COPY . .
ENTRYPOINT ["/build.sh"]
