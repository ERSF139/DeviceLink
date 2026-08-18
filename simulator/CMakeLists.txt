qt_add_executable(simulator
    WIN32
    main.cpp
    sample.h
    device.h
    device.cpp
    simulatorwindow.h
    simulatorwindow.cpp
)

target_link_libraries(simulator
    PRIVATE
        Qt::Core
        Qt::Gui
        Qt::Widgets
)