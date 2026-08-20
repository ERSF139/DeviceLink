qt_add_executable(tst_protocol
    tst_protocol.cpp
)

target_link_libraries(tst_protocol
    PRIVATE
        common
        Qt::Test
)

add_test(NAME tst_protocol COMMAND tst_protocol)