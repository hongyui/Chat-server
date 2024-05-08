cmake_minimum_required(VERSION 3.10)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "Vcpkg toolchain file")
project("chat-server")
set(CMAKE_CXX_STANDARD 17)
set(SRC ${PROJECT_SOURCE_DIR}/src/main.cpp)


find_package(fmt CONFIG REQUIRED)
find_package(range-v3 REQUIRED)
find_package(cxxopts REQUIRED)
add_executable(${PROJECT_NAME} ${SRC})
target_link_libraries(${PROJECT_NAME}
  PRIVATE
    fmt::fmt
    range-v3::range-v3
    cxxopts::cxxopts)



