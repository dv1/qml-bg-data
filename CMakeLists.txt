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

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DQMLBGDATA_DEBUG_BUILD")

# From https://gitlab.kitware.com/vtk/vtk/-/blob/master/CMakeLists.txt
set(CMAKE_INSTALL_QMLDIR "" CACHE PATH "QML files (LIBDIR/qml)")
mark_as_advanced(CMAKE_INSTALL_QMLDIR)
if (NOT CMAKE_INSTALL_QMLDIR)
	set(CMAKE_INSTALL_QMLDIR "${CMAKE_INSTALL_LIBDIR}/qml")
endif ()


include(CheckTypeSize)
include(FindPkgConfig)

# We need floats to be 32-bit for conversions from 4-byte tuples to float
# (See bgdatareceiver.cpp)
check_type_size("float" VAR_FLOAT_SIZE LANGUAGE "CXX")
if (NOT VAR_FLOAT_SIZE EQUAL "4")
	message(FATAL_ERROR "size of float type must be 4 ; actual size: ${VAR_FLOAT_SIZE}")
endif()

find_package(Qt5 COMPONENTS Core DBus Qml Quick REQUIRED)

set(qmlbgdata_SOURCES
	src/bgdatareceiver.cpp
	src/bgdatareceiver.hpp
	src/bgtimeseriesview.cpp
	src/bgtimeseriesview.hpp
	src/qmlbgdataplugin.hpp
	src/qmlbgdataplugin.cpp
)

qt5_add_dbus_adaptor(qmlbgdata_SOURCES src/extappmsgreceiveriface.xml src/bgdatareceiver.hpp BGDataReceiver)

add_library(qmlbgdata SHARED ${qmlbgdata_SOURCES})
target_link_libraries(qmlbgdata Qt5::Core Qt5::DBus Qt5::Qml Qt5::Quick)
target_compile_options(qmlbgdata PRIVATE -Wextra -Wall -pedantic)

set(PLUGIN_PATH ${CMAKE_INSTALL_QMLDIR}/QmlBgData)

install(TARGETS ${PROJECT_NAME} DESTINATION ${PLUGIN_PATH})
install(FILES resources/qml/qmldir DESTINATION ${PLUGIN_PATH})
