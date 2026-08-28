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

qt_add_executable(tst_alarm
    tst_alarm.cpp
)

target_link_libraries(tst_alarm
    PRIVATE
    common
    Qt::Test
)

add_test(NAME tst_alarm COMMAND tst_alarm)

qt_add_executable(tst_sampledatabase tst_sampledatabase.cpp
    ../client/sampledatabase.h
    ../client/sampledatabase.cpp)
target_include_directories(tst_sampledatabase PRIVATE ${CMAKE_SOURCE_DIR}/client)
target_link_libraries(tst_sampledatabase PRIVATE common Qt::Test Qt::Sql)
add_test(NAME tst_sampledatabase COMMAND tst_sampledatabase)

qt_add_executable(tst_reconnect tst_reconnect.cpp)
target_link_libraries(tst_reconnect PRIVATE common Qt::Test)
add_test(NAME tst_reconnect COMMAND tst_reconnect)