cmake_minimum_required(VERSION 3.0)
project(pumila CXX)
if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

find_package(Eigen3 REQUIRED)
include(FindPkgConfig)
pkg_search_module(SDL2 sdl2)
pkg_search_module(SDL2TTF SDL2_ttf>=2.0)
if(SDL2_FOUND AND SDL2TTF_FOUND)
    set(PUMILA_SDL2 1)
    message(STATUS "SDL2 and SDL2TTF found")
else()
    set(PUMILA_SDL2 0)
    message(STATUS "SDL2 and SDL2TTF not found (SDL2_FOUND = '${SDL2_FOUND}', SDL2TTF_FOUND = '${SDL2TTF_FOUND}')")
endif()

add_library(pumila SHARED
    src/pumila/field.cc
    src/pumila/chain.cc
    src/pumila/window.cc
    src/pumila/game.cc
)
target_include_directories(pumila PUBLIC include)
target_compile_features(pumila PUBLIC cxx_std_11)
if(PUMILA_SDL2)
    target_link_directories(pumila PRIVATE ${SDL2_LIBDIR} ${SDL2TTF_LIBDIR})
    target_link_libraries(pumila PRIVATE ${SDL2_LIBRARIES} ${SDL2TTF_LIBRARIES})
    target_compile_definitions(pumila PRIVATE PUMILA_SDL2)
endif()

if(PUMILA_SDL2)
    add_executable(pumila-play src/play/main.cc)
    target_link_libraries(pumila-play PRIVATE pumila)
endif()
