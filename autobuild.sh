#!/bin/bash
git config core.autocrlf false
# 原始碼目錄
PROJECT_DIR=$(realpath ".")
CLIENT_SRC_DIR="${PROJECT_DIR}/src/client"
SERVER_SRC_DIR="${PROJECT_DIR}/src/server"

# 客户端和服务器的构建目录
BUILD_DIR="${PROJECT_DIR}/build"

# 檢查文件變化
function check_changes {
    local src_dir=$1
    local build_dir=$2
    local component_name=$3

    echo "Checking for changes in ${component_name}..."

    # 檢查是否執行cmake configure
    if [ -n "$(git diff --name-only HEAD $src_dir/CMakeLists.txt)" ]; then
        echo "${component_name} CMake configuration changed. Reconfiguring..."
        cmake -B build -S $src_dir
        if [ $? -ne 0 ]; then
            echo "${component_name} CMake configuration failed."
            exit 1
        fi
    fi  

    # 檢查是否有原始碼變化
    if [ -n "$(git diff --name-only HEAD $src_dir/*.cpp $src_dir/*.h)" ]; then
        echo "${component_name} source code changed. Rebuilding..."
        cmake --build build
        if [ $? -ne 0 ]; then
            echo "${component_name} build failed."
            exit 1
        fi
    else
        echo "No changes detected in ${component_name}, skipping build."
    fi
}

# client端
check_changes $CLIENT_SRC_DIR $BUILD_DIR "Client"

# server端
check_changes $SERVER_SRC_DIR $BUILD_DIR "Server"
