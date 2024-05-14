cmake_minimum_required(VERSION 3.18)
project(pumila CXX)
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()
message(STATUS "Build Type = ${CMAKE_BUILD_TYPE}")

if(MSVC)
    foreach(CONF DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONF} ${CMAKE_CURRENT_BINARY_DIR})
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONF} ${CMAKE_CURRENT_BINARY_DIR})
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONF} ${CMAKE_CURRENT_BINARY_DIR})
    endforeach()
    message(${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG})
endif()

include(FetchContent)

message(STATUS "Fetching pybind11 source...")
FetchContent_Declare(pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG stable
)
FetchContent_MakeAvailable(pybind11)

message(STATUS "Fetching BS::thread_pool source...")
FetchContent_Declare(bs_thread_pool
    GIT_REPOSITORY https://github.com/bshoshany/thread-pool.git
    GIT_TAG v4.0.1
)
FetchContent_Populate(bs_thread_pool)
add_library(bs_thread_pool INTERFACE)
target_include_directories(bs_thread_pool INTERFACE ${bs_thread_pool_SOURCE_DIR}/include)

message(STATUS "Fetching cli11 source...")
FetchContent_Declare(cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.3.2
)
FetchContent_MakeAvailable(cli11)

message(STATUS "Fetching roboto font...")
FetchContent_Declare(roboto-font
    URL https://github.com/googlefonts/roboto/releases/download/v2.138/roboto-android.zip
)
FetchContent_Populate(roboto-font)
configure_file(
    ${roboto-font_SOURCE_DIR}/Roboto-Regular.ttf
    ${CMAKE_CURRENT_BINARY_DIR}/Roboto-Regular.ttf
    COPYONLY
)

message(STATUS "Fetching googletest Source...")
FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
set(gtest_force_shared_crt on CACHE INTERNAL "" FORCE)
set(INSTALL_GTEST off CACHE INTERNAL "" FORCE)
FetchContent_MakeAvailable(googletest)

if(MSVC)
    add_compile_options(/W4 /utf-8)
    add_compile_definitions(
        _CRT_SECURE_NO_WARNINGS
        _SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING
    )
else()
    add_compile_options(-Wall -Wpedantic -Wextra)
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -UNDEBUG")
endif()

list(APPEND PUMILA_CORE_SRC
    pumila-core/lib/action.cc
    pumila-core/lib/field3.cc
    pumila-core/lib/chain.cc
    pumila-core/lib/game.cc
    pumila-core/lib/models/pumila14.cc
)
list(APPEND PUMILA_TEST_SRC
    pumila-core/test/field_test.cc
)
if(WIN32)
    list(APPEND PUMILA_CORE_SRC pumila-core/lib/version.rc)
endif()

add_library(pumila-core SHARED ${PUMILA_CORE_SRC})
target_include_directories(pumila-core PUBLIC pumila-core/include)
target_compile_features(pumila-core PUBLIC cxx_std_20)
target_link_libraries(pumila-core PUBLIC bs_thread_pool)
if(MSVC)
    set_target_properties(pumila-core PROPERTIES DEBUG_POSTFIX d)
endif()

pybind11_add_module(pypumila pumila-core/pypumila.cc)
target_link_libraries(pypumila PUBLIC pumila-core)

enable_testing()
add_executable(pumila-test ${PUMILA_TEST_SRC})
target_link_libraries(pumila-test PRIVATE pumila-core GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(pumila-test DISCOVERY_TIMEOUT 600)
