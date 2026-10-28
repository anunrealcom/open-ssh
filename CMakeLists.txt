cmake_minimum_required(VERSION 3.16)
project(xiaobaiOP VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT Qt6_DIR AND DEFINED ENV{QT6_DIR})
    set(Qt6_DIR "$ENV{QT6_DIR}")
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

add_executable(xiaobaiOP
    src/main.cpp
    src/session.h
    src/session.cpp
    src/conn.h
    src/sshsession.h
    src/sshsession.cpp
    src/serialsession.h
    src/serialsession.cpp
    src/filetransfer.h
    src/filetransfer.cpp
    src/terminalwidget.h
    src/terminalwidget.cpp
    src/sessiondialog.h
    src/sessiondialog.cpp
    src/tunnel.h
    src/tunnel.cpp
    src/tunneldialog.h
    src/tunneldialog.cpp
    src/serialdialog.h
    src/serialdialog.cpp
    src/filewidget.h
    src/filewidget.cpp
    src/framelessdialog.h
    src/framelessdialog.cpp
    src/xbmessagebox.h
    src/xbmessagebox.cpp
    src/ai.h
    src/ai.cpp
    src/aidialog.h
    src/aidialog.cpp
    src/aichat.h
    src/aichat.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    src/app.rc
)

target_link_libraries(xiaobaiOP PRIVATE Qt6::Widgets Qt6::Network)
target_include_directories(xiaobaiOP PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(WIN32)
    # 隐藏控制台窗口（纯 GUI 程序）
    set_target_properties(xiaobaiOP PROPERTIES WIN32_EXECUTABLE ON)
endif()
