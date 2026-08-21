qt_add_executable(tst_protocol
    tst_protocol.cpp
)

target_link_libraries(tst_protocol
    PRIVATE
        common
        Qt::Test
)

add_test(NAME tst_protocol COMMAND tst_protocol)

qt_add_executable(tst_frameparser
    tst_frameparser.cpp
)

target_link_libraries(tst_frameparser
    PRIVATE
        common
        Qt::Test
)

add_test(NAME tst_frameparser COMMAND tst_frameparser)