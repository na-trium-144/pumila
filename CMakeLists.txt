cmake_minimum_required(VERSION 3.0)
project(pumila CXX)
if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

find_package(Eigen3 REQUIRED)
include(FindPkgConfig)
pkg_search_module(SDL2 sdl2)
pkg_search_module(SDL2TTF SDL2_ttf>=2.0)

add_library(pumila SHARED
    src/pumila/field.cc
    src/pumila/chain.cc
    src/pumila/window.cc
    src/pumila/game.cc
)
target_include_directories(pumila PUBLIC include)
target_compile_features(pumila PUBLIC cxx_std_11)
target_link_libraries(pumila PRIVATE ${SDL2_LIBRARIES} ${SDL2TTF_LIBRARIES})
target_compile_definitions(pumila PRIVATE
    PUMILA_SDL2=${SDL2_FOUND}
)

add_executable(pumila-play src/play/main.cc)
target_link_libraries(pumila-play PRIVATE pumila)
