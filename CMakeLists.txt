cmake_minimum_required(VERSION 3.8.0)

project(qmlbgdata
	VERSION 0.0.1
	LANGUAGES CXX
	DESCRIPTION "QML plugin for receiving blood glucose (BG) data over D-Bus")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

include(FindPkgConfig)

find_package(Qt5 COMPONENTS Core DBus Qml REQUIRED)

set(qmlbgdata_SOURCES
	src/bgdatareceiver.cpp
	src/bgdatareceiver.hpp
	src/qmlbgdataplugin.hpp
	src/qmlbgdataplugin.cpp
)

qt5_add_dbus_adaptor(qmlbgdata_SOURCES src/extappmsgreceiveriface.xml src/bgdatareceiver.hpp BGDataReceiver)

add_library(qmlbgdata SHARED ${qmlbgdata_SOURCES})
target_link_libraries(qmlbgdata Qt5::Core Qt5::DBus Qt5::Qml)
target_compile_options(qmlbgdata PRIVATE -Wextra -Wall -pedantic)

set(PLUGIN_PATH ${QT_DIR}/../../../qml/${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME} DESTINATION ${PLUGIN_PATH})
install(FILES resources/qml/qmldir DESTINATION ${PLUGIN_PATH})
