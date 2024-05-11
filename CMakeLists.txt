cmake_minimum_required(VERSION 3.10)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "Vcpkg toolchain file")
project("chat-server")
set(CMAKE_CXX_STANDARD 17)
set(SRC ${PROJECT_SOURCE_DIR}/src/main.cpp)

find_package(Boost REQUIRED COMPONENTS system type_traits)
find_package(asio CONFIG REQUIRED)
#find_package(Boost REQUIRED COMPONENTS random)
find_package(websocketpp CONFIG REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
add_executable(${PROJECT_NAME} ${SRC})

# Remove the pthread dependency
# set(LIB pthread)  

target_link_libraries(${PROJECT_NAME} PRIVATE websocketpp::websocketpp asio asio::asio ${Boost_LIBRARIES})
