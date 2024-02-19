cmake_minimum_required(VERSION 3.23)
project(curaengine_onlyfans)
find_package(standardprojectsettings REQUIRED)

find_package(Protobuf REQUIRED)
find_package(spdlog REQUIRED)
find_package(docopt REQUIRED)
find_package(asio-grpc REQUIRED)
find_package(range-v3 REQUIRED)
find_package(ctre REQUIRED)
find_package(semver REQUIRED)
find_package(curaengine_grpc_definitions REQUIRED)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(HDRS include/plugin/broadcast.h
        include/plugin/cmdline.h
        include/plugin/handshake.h
        include/plugin/metadata.h
        include/plugin/modify.h
        include/plugin/plugin.h
        include/plugin/settings.h
        include/processor/process.h)

add_library(curaengine_onlyfans_lib INTERFACE ${HDRS})
use_threads(curaengine_onlyfans_lib)
target_link_libraries(curaengine_onlyfans_lib INTERFACE curaengine_grpc_definitions::curaengine_grpc_definitions ctre::ctre asio-grpc::asio-grpc protobuf::libprotobuf boost::boost spdlog::spdlog docopt_s range-v3::range-v3 semver::semver)
target_include_directories(curaengine_onlyfans_lib
        INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)



add_executable(curaengine_onlyfans src/main.cpp)
use_threads(curaengine_onlyfans)
target_link_libraries(curaengine_onlyfans PUBLIC curaengine_onlyfans_lib)



enable_testing()

add_executable(test_process src/test.cpp)
#add_test(NAME check COMMAND tests)
use_threads(curaengine_onlyfans)
#    ${CMAKE_CURRENT_SOURCE_DIR}/../src/include
#    ${CMAKE_CURRENT_SOURCE_DIR}/../src
#)
target_link_libraries(test_process PUBLIC curaengine_onlyfans_lib  GTest::gtest_main)
target_compile_options(test_process PRIVATE -DVKB_WARNINGS_AS_ERRORS=OFF)
