cmake_minimum_required(VERSION 3.16)

project(PardusLanShare VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(PardusLanShare
    main.cpp
    pardus_server.cpp
    pardus_server.h
    device_discovery.cpp
    device_discovery.h
)

target_link_libraries(PardusLanShare PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)