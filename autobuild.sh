#!/bin/bash
#git config core.autocrlf false
# 原始碼目錄
PROJECT_DIR=$(realpath ".")
CLIENT_SRC_DIR="${PROJECT_DIR}/src/client"
SERVER_SRC_DIR="${PROJECT_DIR}/src/server"

#  build目錄
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
        cmake -B $build_dir -S $src_dir
        if [ $? -ne 0 ]; then
            echo "${component_name} CMake configuration failed."
            exit 1
        fi
    fi  

    # 檢查是否有原始碼變化
    if [ -n "$(git diff --name-only HEAD $src_dir/*.cpp $src_dir/*.h)" ]; then
        echo "${component_name} source code changed. Rebuilding..."
        cmake --build $build_dir
        if [ $? -ne 0 ]; then
            echo "${component_name} build failed."
            exit 1
        fi
    else
        echo "No changes detected in ${component_name}, skipping build."
    fi
}


# 檢查build目錄是否有檔案，如果沒有，則進行vcpkg和cmake初始化
function setup_build_and_vcpkg {
    if [ ! -d "$BUILD_DIR" ] || [ -z "$(ls -A $BUILD_DIR)" ]; then
        echo "Build directory is empty or not present. Setting up build environment..."

        # 檢查並設置vcpkg
        if [ ! -d "./vcpkg" ]; then
            echo "vcpkg directory not found. Cloning and setting up vcpkg..."
            git clone https://github.com/Microsoft/vcpkg
            ./vcpkg/bootstrap-vcpkg.sh
            if [ $? -ne 0 ]; then
                echo "Failed to setup vcpkg."
                exit 1
            fi
        else
            echo "vcpkg directory already exists."
        fi

        # 初始化build目錄
        cmake -B $BUILD_DIR -S .
        if [ $? -ne 0 ]; then
            echo "Failed to initialize build directory."
            exit 1
        fi
        cmake --build $BUILD_DIR
        if [ $? -ne 0 ]; then
            echo "Failed to build initial project."
            exit 1
        fi
    fi
}

# 設置build目錄和vcpkg
setup_build_and_vcpkg


check_changes $CLIENT_SRC_DIR $BUILD_DIR "Client"
check_changes $SERVER_SRC_DIR $BUILD_DIR "Server"
