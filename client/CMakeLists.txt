qt_add_executable(client
    WIN32
    main.cpp
    mainwindow.h
    mainwindow.cpp
)

target_link_libraries(client
    PRIVATE
        Qt::Core
        Qt::Gui
        Qt::Widgets
)