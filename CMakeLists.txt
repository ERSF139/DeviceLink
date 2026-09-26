cmake_minimum_required(VERSION 3.19)

project(DeviceLink LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets Network Test Charts Sql)

qt_standard_project_setup()

# 可执行文件不能直接放在构建根目录：Linux 下没有 .exe 后缀，
# client、simulator 会和同名的子目录构建目录冲突
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

enable_testing()

add_subdirectory(common)
add_subdirectory(simulator)
add_subdirectory(client)
add_subdirectory(tests)
