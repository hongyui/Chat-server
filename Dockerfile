FROM ubuntu:latest


ENV DEBIAN_FRONTEND=noninteractive

# 安装基本工具
RUN apt-get update
RUN apt install -y git curl wget vim gdb gnupg software-properties-common build-essential zip unzip tar pkg-config inotify-tools && rm -rf /var/lib/apt/lists/*
RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | apt-key add -
RUN apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main'
RUN apt-get update
RUN apt install -y cmake

# 安装 Vcpkg
RUN git clone https://github.com/Microsoft/vcpkg.git /chatserver/vcpkg
RUN /chatserver/vcpkg/bootstrap-vcpkg.sh

# 設定Vcpkg路徑
ENV CMAKE_TOOLCHAIN_FILE=/chatserver/vcpkg/scripts/buildsystems/vcpkg.cmake

# 預設目錄
WORKDIR /chatserver
RUN git config --global core.autocrlf false

# 容器啟動時新增終端機
CMD ["/bin/bash"]
