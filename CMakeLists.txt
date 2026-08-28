cmake_minimum_required(VERSION 3.16)
project(aeroboard LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(Protobuf REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(ZMQ    REQUIRED libzmq)
pkg_check_modules(LIBEVENT REQUIRED libevent)
find_package(CURL REQUIRED)

find_package(OpenCV REQUIRED)

# ── Shared proto library ──────────────────────────────────────────────────────
protobuf_generate_cpp(PROTO_SRC PROTO_HDR proto/screen_event.proto)
add_library(aeroboard_proto STATIC ${PROTO_SRC} ${PROTO_HDR})
target_include_directories(aeroboard_proto PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Protobuf_INCLUDE_DIRS})
target_link_libraries(aeroboard_proto PUBLIC ${Protobuf_LIBRARIES})

# ── Common ────────────────────────────────────────────────────────────────────
set(COMMON_INCLUDES
    ${CMAKE_CURRENT_BINARY_DIR}
    ${ZMQ_INCLUDE_DIRS}
    ${LIBEVENT_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    ${Protobuf_INCLUDE_DIRS})

set(COMMON_LIBS
    aeroboard_proto
    ${ZMQ_LIBRARIES}
    ${LIBEVENT_LIBRARIES}
    ${CURL_LIBRARIES}
    ${Protobuf_LIBRARIES})

# ── EventHub ──────────────────────────────────────────────────────────────────
add_executable(eventhub
    EventHub/src/main.cpp
    EventHub/src/hub_client.cpp
    EventHub/src/event_dispatcher.cpp
    EventHub/src/event_logger.cpp
    EventHub/src/cloud_poster.cpp
    EventHub/src/hub_config.cpp
    EventHub/src/frame_reader.cpp
    EventHub/src/worker_manager.cpp
    EventHub/src/settings_forwarder.cpp)
target_include_directories(eventhub PRIVATE EventHub/src ${COMMON_INCLUDES})
target_link_libraries(eventhub PRIVATE ${COMMON_LIBS})

# ── VideoWorker ───────────────────────────────────────────────────────────────
add_executable(aeroboard-video
    VideoWorker/src/main.cpp
    VideoWorker/src/frame_reader.cpp
    VideoWorker/src/video_config.cpp
    VideoWorker/src/video_publisher.cpp)
target_include_directories(aeroboard-video PRIVATE VideoWorker/src ${COMMON_INCLUDES})
target_link_libraries(aeroboard-video PRIVATE ${COMMON_LIBS})

# ── SensorWorker ──────────────────────────────────────────────────────────────
add_executable(aeroboard-sensor
    SensorWorker/src/main.cpp
    SensorWorker/src/sensor_config.cpp
    SensorWorker/src/sensor_publisher.cpp)
target_include_directories(aeroboard-sensor PRIVATE SensorWorker/src ${COMMON_INCLUDES})
target_link_libraries(aeroboard-sensor PRIVATE ${COMMON_LIBS})

# ── SystemWorker ──────────────────────────────────────────────────────────────
add_executable(aeroboard-system
    SystemWorker/src/main.cpp
    SystemWorker/src/system_config.cpp)
target_include_directories(aeroboard-system PRIVATE SystemWorker/src ${COMMON_INCLUDES})
target_link_libraries(aeroboard-system PRIVATE ${COMMON_LIBS})

# ── AIWorker ──────────────────────────────────────────────────────────────────
add_executable(aeroboard-ai
    AIWorker/src/main.cpp
    AIWorker/src/detector.cpp
    AIWorker/src/ai_config.cpp)
target_include_directories(aeroboard-ai PRIVATE AIWorker/src ${COMMON_INCLUDES} ${OpenCV_INCLUDE_DIRS})
target_link_libraries(aeroboard-ai PRIVATE ${COMMON_LIBS} ${OpenCV_LIBS})

# ── GUI (Qt5/QML) ─────────────────────────────────────────────────────────────
find_package(Qt5 COMPONENTS Core Gui Quick QuickControls2 QUIET)

if(Qt5_FOUND)
    message(STATUS "Qt5 found (${Qt5_VERSION}) — building gui-app")

    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)

    add_executable(gui-app
        Gui/src/main.cpp
        Gui/src/backend.cpp
        Gui/src/app_config.cpp
        Gui/src/hub_publisher.cpp
        Gui/src/video_item.cpp
        Gui/resources.qrc)

    target_include_directories(gui-app PRIVATE
        Gui/src
        ${CMAKE_CURRENT_BINARY_DIR}
        ${ZMQ_INCLUDE_DIRS}
        ${Protobuf_INCLUDE_DIRS})

    target_link_libraries(gui-app PRIVATE
        Qt5::Core
        Qt5::Gui
        Qt5::Quick
        Qt5::QuickControls2
        aeroboard_proto
        ${ZMQ_LIBRARIES}
        ${Protobuf_LIBRARIES})

    # Yocto SDK: tell Qt where to find platform plugins at runtime
    if(DEFINED ENV{SDKTARGETSYSROOT})
        target_compile_definitions(gui-app PRIVATE
            QT_QPA_PLATFORM_PLUGIN_PATH="$ENV{SDKTARGETSYSROOT}/usr/lib/qt5/plugins")
    endif()

    install(TARGETS gui-app RUNTIME DESTINATION /usr/bin)
else()
    message(WARNING "Qt5 not found — skipping gui-app (source SDK env first)")
endif()

# ── Tests ─────────────────────────────────────────────────────────────────────
option(BUILD_TESTING "Build unit tests" ON)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

# ── Install ───────────────────────────────────────────────────────────────────
install(DIRECTORY config/ DESTINATION /etc/aeroboard)
install(TARGETS eventhub aeroboard-video aeroboard-sensor
                aeroboard-system aeroboard-ai
        RUNTIME DESTINATION /usr/bin)