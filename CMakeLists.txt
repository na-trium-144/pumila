cmake_minimum_required(VERSION 3.11)
project(pumila CXX)
if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

include(FindPkgConfig)
include(FetchContent)

find_package(Eigen3 QUIET)
if(Eigen3_FOUND)
    message(STATUS "Eigen3 found")
else()
    message(STATUS "Eigen3 not found, fetching source...")
    FetchContent_Declare(Eigen3
        GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
        GIT_TAG 3.4
    )
    FetchContent_MakeAvailable(Eigen3)
endif()

pkg_search_module(SDL2 sdl2)
pkg_search_module(SDL2TTF SDL2_ttf>=2.0)
if(SDL2_FOUND AND SDL2TTF_FOUND)
    set(PUMILA_SDL2 1)
    message(STATUS "SDL2 and SDL2TTF found")
else()
    set(PUMILA_SDL2 0)
    message(STATUS "SDL2 and SDL2TTF not found (SDL2_FOUND = '${SDL2_FOUND}', SDL2TTF_FOUND = '${SDL2TTF_FOUND}')")
endif()

message(STATUS "Fetching pybind11 source...")
FetchContent_Declare(pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG stable
)
FetchContent_MakeAvailable(pybind11)

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

add_library(pumila SHARED
    src/pumila/field.cc
    src/pumila/chain.cc
    src/pumila/window.cc
    src/pumila/game.cc
    src/pumila/models/pumila1.cc
)
target_include_directories(pumila PUBLIC include)
target_compile_features(pumila PUBLIC cxx_std_17)
target_link_libraries(pumila PUBLIC Eigen3::Eigen)
if(PUMILA_SDL2)
    target_link_directories(pumila PRIVATE ${SDL2_LIBDIR} ${SDL2TTF_LIBDIR})
    target_link_libraries(pumila PRIVATE ${SDL2_LIBRARIES} ${SDL2TTF_LIBRARIES})
    target_compile_definitions(pumila PRIVATE PUMILA_SDL2)
endif()

pybind11_add_module(pypumila src/pypumila/pypumila.cc)
target_link_libraries(pypumila PUBLIC pumila)

if(PUMILA_SDL2)
    add_executable(pumila-play src/play/main.cc)
    target_link_libraries(pumila-play PRIVATE pumila)
endif()
