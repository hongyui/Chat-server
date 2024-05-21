# 設定cmake版本
cmake_minimum_required(VERSION 3.10)

# 設定vcpkg路徑
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "Vcpkg toolchain file")

# 設定專案名稱
project("ChatApplication") 

# 設定C++版本
set(CMAKE_CXX_STANDARD 17) 

# 設定輸出路徑
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 查找套件庫
find_package(Boost REQUIRED COMPONENTS system type_traits)
find_package(asio CONFIG REQUIRED)
find_package(websocketpp CONFIG REQUIRED)

# 設定Boost的依賴庫
include_directories(${Boost_INCLUDE_DIRS})

# 加載子目錄
add_subdirectory(src)
