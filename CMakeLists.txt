cmake_minimum_required(VERSION 3.16)
project(BinGoat C)

include(FetchContent)
include(CTest)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static dependencies" FORCE)
# CivetWeb 1.16 declares CMake 3.3; keep it configurable on CMake 4.x.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(CIVETWEB_VERSION 1.16.0 CACHE STRING "Pinned CivetWeb version" FORCE)

# Pin dependency versions so local and CI builds are reproducible.
set(CIVETWEB_ENABLE_CXX OFF CACHE BOOL "" FORCE)
set(CIVETWEB_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(CIVETWEB_ENABLE_SERVER_EXECUTABLE OFF CACHE BOOL "" FORCE)
set(CIVETWEB_ENABLE_SSL OFF CACHE BOOL "" FORCE)
set(ENABLE_CJSON_TEST OFF CACHE BOOL "" FORCE)
set(ENABLE_CJSON_UNINSTALL OFF CACHE BOOL "" FORCE)
set(ENABLE_CJSON_UTILS OFF CACHE BOOL "" FORCE)
set(ENABLE_CJSON_VERSION_SO OFF CACHE BOOL "" FORCE)
set(CJSON_OVERRIDE_BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
set(CJSON_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    civetweb
    GIT_REPOSITORY https://github.com/civetweb/civetweb.git
    GIT_TAG v1.16
    GIT_SHALLOW TRUE
)
FetchContent_Declare(
    cjson
    GIT_REPOSITORY https://github.com/DaveGamble/cJSON.git
    GIT_TAG v1.7.19
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(civetweb cjson)

find_package(Threads REQUIRED)

add_library(bingo_engine STATIC bingo_engine.c)
target_include_directories(bingo_engine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
if(NOT MSVC)
    target_compile_options(bingo_engine PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(bingo-server backend/server.c)
target_link_libraries(bingo-server PRIVATE
    bingo_engine
    civetweb-c-library
    cjson
    Threads::Threads
)
target_include_directories(bingo-server PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${civetweb_SOURCE_DIR}/include"
    "${cjson_SOURCE_DIR}"
)
if(NOT MSVC)
    target_compile_options(bingo-server PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS bingo-server RUNTIME DESTINATION bin)
