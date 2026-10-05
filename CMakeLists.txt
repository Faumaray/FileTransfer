cmake_minimum_required(VERSION 3.13)
project(FTU VERSION 2.4.0 LANGUAGES CXX)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "FTU requires Linux (epoll, signalfd, Unix domain sockets)")
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(FTU_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(FTU_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

find_package(Threads REQUIRED)

file(GLOB_RECURSE FTU_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE FTU_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")
list(FILTER FTU_SOURCES EXCLUDE REGEX "/src/main\\.cpp$")

add_library(FTU_options INTERFACE)
target_compile_features(FTU_options INTERFACE cxx_std_20)
target_compile_options(FTU_options INTERFACE
    -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow
    $<$<BOOL:${FTU_WARNINGS_AS_ERRORS}>:-Werror>)
target_compile_definitions(FTU_options INTERFACE _GNU_SOURCE _FILE_OFFSET_BITS=64)
if(FTU_SANITIZERS)
    target_compile_options(FTU_options INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(FTU_options INTERFACE -fsanitize=address,undefined)
endif()

add_library(FTU_core STATIC ${FTU_SOURCES} ${FTU_HEADERS})
target_include_directories(FTU_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(FTU_core PUBLIC Threads::Threads FTU_options)
add_executable(FTU src/main.cpp)
target_link_libraries(FTU PRIVATE FTU_core)
set_target_properties(FTU PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
install(TARGETS FTU RUNTIME DESTINATION bin)