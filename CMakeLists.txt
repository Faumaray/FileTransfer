cmake_minimum_required(VERSION 3.20)
project(TrasferFileUtility VERSION 1.0.0 LANGUAGES CXX)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "TrasferFileUtility requires Linux")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTING "Build and register tests" OFF)
option(TFU_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)
option(TFU_ENABLE_SANITIZERS "Enable AddressSanitizer and UBSan" OFF)

include(cmake/CompilerOptions.cmake)
find_package(Threads REQUIRED)

file(GLOB_RECURSE VOID_SOURCES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/src/void/*.cpp"
    "${PROJECT_SOURCE_DIR}/include/void/*.hpp"
)
file(GLOB_RECURSE PROTOCOL_SOURCES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/src/protocol/*.cpp"
    "${PROJECT_SOURCE_DIR}/include/protocol/*.hpp"
)
file(GLOB_RECURSE SERVER_SOURCES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/src/server/*.cpp"
    "${PROJECT_SOURCE_DIR}/include/server/*.hpp"
)
file(GLOB_RECURSE CLIENT_SOURCES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/src/client/*.cpp"
    "${PROJECT_SOURCE_DIR}/include/client/*.hpp"
)

add_library(void_library STATIC ${PROTOCOL_SOURCES})
target_include_directories(void_library PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_compile_definitions(void_library PUBLIC _GNU_SOURCE _FILE_OFFSET_BITS=64)
target_compile_features(void_library PUBLIC cxx_std_20)
target_link_libraries(void_library PUBLIC Threads::Threads tfu_options)

add_library(${CMAKE_PROJECT_NAME}_protocol STATIC ${PROTOCOL_SOURCES})
target_include_directories(${CMAKE_PROJECT_NAME}_protocol PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_compile_definitions(${CMAKE_PROJECT_NAME}_protocol PUBLIC _GNU_SOURCE _FILE_OFFSET_BITS=64)
target_compile_features(${CMAKE_PROJECT_NAME}_protocol PUBLIC cxx_std_20)
target_link_libraries(${CMAKE_PROJECT_NAME}_protocol PUBLIC Threads::Threads void_library tfu_options)

add_library(${CMAKE_PROJECT_NAME}_server_library STATIC ${SERVER_SOURCES})
target_link_libraries(${CMAKE_PROJECT_NAME}_server_library PUBLIC ${CMAKE_PROJECT_NAME}_protocol)

add_library(${CMAKE_PROJECT_NAME}_client_library STATIC ${CLIENT_SOURCES})
target_link_libraries(${CMAKE_PROJECT_NAME}_client_library PUBLIC ${CMAKE_PROJECT_NAME}_protocol)

add_executable(${CMAKE_PROJECT_NAME} src/main.cpp)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_PROJECT_NAME}_server_library ${CMAKE_PROJECT_NAME}_client_library)
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

include(GNUInstallDirs)
install(TARGETS ${CMAKE_PROJECT_NAME} RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
