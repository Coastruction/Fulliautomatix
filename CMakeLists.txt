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

set(HDRS include/plugin/broadcast.h
        include/plugin/cmdline.h
        include/plugin/handshake.h
        include/plugin/metadata.h
        include/plugin/modify.h
        include/plugin/plugin.h
        include/plugin/settings.h
        include/plugin/process.h)

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

add_executable(test_process src/test.cpp)
use_threads(curaengine_onlyfans)
target_link_libraries(test_process PUBLIC curaengine_onlyfans_lib)


