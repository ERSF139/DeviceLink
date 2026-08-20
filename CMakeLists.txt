cmake_minimum_required(VERSION 3.19)

project(DeviceLink LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets Network Test)

qt_standard_project_setup()

enable_testing()

add_subdirectory(common)
add_subdirectory(simulator)
add_subdirectory(client)
add_subdirectory(tests)
