cmake_minimum_required(VERSION 3.16)
project(onedge LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ONEDGE_BUILD_TESTS    "Build the onedge test suite"    ON)
option(ONEDGE_BUILD_EXAMPLES "Build the onedge examples"      ON)
option(ONEDGE_SANITIZERS     "Enable ASAN and UBSAN (Linux/Clang/GCC)" OFF)
option(ONEDGE_WERROR         "Treat warnings as errors"       OFF)

add_library(onedge STATIC
    src/error.cpp
    src/buffer.cpp
    src/string.cpp
    src/c_api.cpp
)

target_include_directories(onedge
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(MSVC)
    target_compile_options(onedge PRIVATE /W4 /permissive-)
    if(ONEDGE_WERROR)
        target_compile_options(onedge PRIVATE /WX)
    endif()
    # Hush unhelpful warnings about safe stdlib usage where we explicitly
    # use the standard versions on purpose.
    target_compile_definitions(onedge PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(onedge PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    if(ONEDGE_WERROR)
        target_compile_options(onedge PRIVATE -Werror)
    endif()
endif()

if(ONEDGE_SANITIZERS)
    if(NOT MSVC)
        target_compile_options(onedge PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(onedge PUBLIC -fsanitize=address,undefined)
    else()
        message(WARNING "ONEDGE_SANITIZERS not supported on MSVC in this build script")
    endif()
endif()

if(ONEDGE_BUILD_TESTS)
    enable_testing()
    add_executable(onedge_tests
        tests/test_main.cpp
        tests/test_checked_math.cpp
        tests/test_result.cpp
        tests/test_buffer.cpp
        tests/test_string.cpp
        tests/test_c_api.cpp
        tests/test_span.cpp
        tests/test_reader.cpp
        tests/test_writer.cpp
        tests/test_fixed_buffer.cpp
        tests/test_secure_types.cpp
    )
    target_link_libraries(onedge_tests PRIVATE onedge)
    target_include_directories(onedge_tests PRIVATE tests)
    if(MSVC)
        target_compile_options(onedge_tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(onedge_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME onedge_tests COMMAND onedge_tests)
endif()

if(ONEDGE_BUILD_EXAMPLES)
    add_executable(example_safe_string examples/safe_string.cpp)
    target_link_libraries(example_safe_string PRIVATE onedge)

    add_executable(example_safe_buffer examples/safe_buffer.cpp)
    target_link_libraries(example_safe_buffer PRIVATE onedge)

    add_executable(example_parse_framed_message examples/parse_framed_message.cpp)
    target_link_libraries(example_parse_framed_message PRIVATE onedge)
endif()
