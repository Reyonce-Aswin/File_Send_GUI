cmake_minimum_required(VERSION 3.16)
project(FileSend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

set(CMAKE_AUTORCC ON)
if(WIN32)
    enable_language(RC)
endif()

# Works with Qt 6 or Qt 5.15+
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

add_executable(filesend
    main.cpp
    mainwindow.h mainwindow.cpp
    discovery.h discovery.cpp
    filesender.h filesender.cpp
    filereceiver.h filereceiver.cpp
    common.h
    resources.qrc
)
if(WIN32)
    target_sources(filesend PRIVATE app.rc)
endif()

target_link_libraries(filesend PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network)

set_target_properties(filesend PROPERTIES WIN32_EXECUTABLE ON)
install(TARGETS filesend RUNTIME DESTINATION bin)
