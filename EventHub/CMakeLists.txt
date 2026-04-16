cmake_minimum_required(VERSION 3.16)
project(eventhub CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(Protobuf REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBEVENT REQUIRED libevent)
pkg_check_modules(LIBCURL  REQUIRED libcurl)

include_directories(${CMAKE_SOURCE_DIR}/../EventHub/build)

# Generate protobuf sources
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS proto/screen_event.proto)

add_executable(eventhub
    src/main.cpp
    src/frame_reader.cpp
    src/cloud_poster.cpp
    src/event_logger.cpp
    src/event_dispatcher.cpp
    ${PROTO_SRCS}
)

add_library(eventhub_proto
    screen_event.pb.cc
)

target_include_directories(eventhub PRIVATE
    src
    ${CMAKE_CURRENT_BINARY_DIR}   # generated .pb.h
    ${Protobuf_INCLUDE_DIRS}
    ${LIBEVENT_INCLUDE_DIRS}
    ${LIBCURL_INCLUDE_DIRS}
)

target_link_libraries(eventhub PRIVATE
    ${Protobuf_LIBRARIES}
    ${LIBEVENT_LIBRARIES}
    ${LIBCURL_LIBRARIES}
    pthread
)