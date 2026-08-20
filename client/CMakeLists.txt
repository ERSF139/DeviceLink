qt_add_executable(client
    WIN32
    main.cpp
    mainwindow.h
    mainwindow.cpp
    deviceclient.h
    deviceclient.cpp
)

target_link_libraries(client
    PRIVATE
        common
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt6::Network
)
