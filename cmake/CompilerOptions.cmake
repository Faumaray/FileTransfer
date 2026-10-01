add_library(${CMAKE_PROJECT_NAME}_options INTERFACE)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${CMAKE_PROJECT_NAME}_options INTERFACE
        -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow
        -Wformat=2 -Wnull-dereference
    )
    if(${CMAKE_PROJECT_NAME}_WARNINGS_AS_ERRORS)
        target_compile_options(${CMAKE_PROJECT_NAME}_options INTERFACE -Werror)
    endif()
    if(${CMAKE_PROJECT_NAME}_ENABLE_SANITIZERS)
        target_compile_options(${CMAKE_PROJECT_NAME}_options INTERFACE
            -fsanitize=address,undefined -fno-omit-frame-pointer
        )
        target_link_options(${CMAKE_PROJECT_NAME}_options INTERFACE -fsanitize=address,undefined)
    endif()
endif()
