find_package(nlohmann_json REQUIRED)

add_executable(unit_tests
    test_config.cpp
    ${CMAKE_SOURCE_DIR}/SystemWorker/src/system_config.cpp
    ${CMAKE_SOURCE_DIR}/VideoWorker/src/video_config.cpp)

target_include_directories(unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/SystemWorker/src
    ${CMAKE_SOURCE_DIR}/VideoWorker/src)

target_link_libraries(unit_tests PRIVATE nlohmann_json::nlohmann_json pthread)

add_test(NAME unit_tests COMMAND unit_tests)
