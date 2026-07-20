cmake_minimum_required(VERSION 3.24)

project(
    SabTrace
    VERSION 0.1.0
    DESCRIPTION "Fast, script-friendly log filtering and statistics"
    LANGUAGES CXX
)

option(SABTRACE_BUILD_TESTS "Build the SabTrace test executable" ON)

add_library(sabtrace_core
    src/analyzer.cpp
    src/cli.cpp
)
add_library(SabTrace::core ALIAS sabtrace_core)

target_compile_features(sabtrace_core PUBLIC cxx_std_20)
target_include_directories(sabtrace_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(MSVC)
    target_compile_options(sabtrace_core PRIVATE /W4 /permissive- /EHsc)
else()
    target_compile_options(sabtrace_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

add_executable(sabtrace src/main.cpp)
target_link_libraries(sabtrace PRIVATE SabTrace::core)
target_compile_features(sabtrace PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(sabtrace PRIVATE /W4 /permissive- /EHsc)
else()
    target_compile_options(sabtrace PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

include(GNUInstallDirs)
install(TARGETS sabtrace RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES README.md LICENSE CHANGELOG.md DESTINATION .)

if(SABTRACE_BUILD_TESTS)
    include(CTest)
    enable_testing()

    add_executable(sabtrace_tests tests/test_main.cpp)
    target_link_libraries(sabtrace_tests PRIVATE SabTrace::core)
    target_compile_features(sabtrace_tests PRIVATE cxx_std_20)

    if(MSVC)
        target_compile_options(sabtrace_tests PRIVATE /W4 /permissive- /EHsc)
    else()
        target_compile_options(sabtrace_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    endif()

    add_test(NAME sabtrace_tests COMMAND sabtrace_tests)
endif()
