qt_add_library(common STATIC
    sample.h
    protocol.h
    protocol.cpp
    frameparser.h
    frameparser.cpp
    alarm.h alarm.cpp
)

target_include_directories(common
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(common
    PUBLIC
    Qt::Core
)