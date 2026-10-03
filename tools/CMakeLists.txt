qt_add_executable(golden_dump golden_dump.cpp)
target_link_libraries(golden_dump PRIVATE common Qt::Core)