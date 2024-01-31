cmake_minimum_required(VERSION 3.18)
project(pumila CXX)
if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

option(PUMILA_MKL off "Use MKL (fot Intel CPU on Linux)")
option(PUMILA_ACCELERATE off "Use Accelerate (for MacOS)")

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
    message(STATUS "sdl2 and SDL2_ttf found")
else()
    set(PUMILA_SDL2 0)
    message(STATUS "sdl2 and SDL2_ttf not found (SDL2_FOUND = '${SDL2_FOUND}', SDL2TTF_FOUND = '${SDL2TTF_FOUND}')")
endif()

if(PUMILA_MKL)
    pkg_search_module(MKL REQUIRED mkl-dynamic-lp64-iomp)
    message(STATUS "mkl-dynamic-lp64-iomp found")
endif()

if(PUMILA_ACCELERATE)
    find_library(ACCELERATE Accelerate REQUIRED)
    message(STATUS "Accelerate found")
endif()

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

add_compile_options(-Wall -Wpedantic -Wextra)

add_library(pumila SHARED
    src/pumila/action.cc
    src/pumila/field.cc
    src/pumila/chain.cc
    src/pumila/window.cc
    src/pumila/game.cc
    src/pumila/model_base.cc
    src/pumila/models/pumila1.cc
    src/pumila/models/pumila1n.cc
    src/pumila/models/pumila2.cc
    src/pumila/models/pumila3.cc
)
target_include_directories(pumila PUBLIC include)
target_compile_features(pumila PUBLIC cxx_std_17)
target_link_libraries(pumila PUBLIC Eigen3::Eigen bs_thread_pool)
if(PUMILA_MKL)
    target_link_directories(pumila PUBLIC ${MKL_LIBDIR})
    target_include_directories(pumila PUBLIC ${MKL_INCLUDE_DIRS})
    target_link_libraries(pumila PUBLIC ${MKL_LIBRARIES} ${MKL_LDFLAGS})
    # target_link_options(pumila PUBLIC ${MKL_LDFLAGS})
    target_compile_definitions(pumila PUBLIC EIGEN_USE_MKL_ALL)
endif()
if(PUMILA_ACCELERATE)
    target_link_libraries(pumila PUBLIC ${ACCELERATE})
    target_compile_definitions(pumila PUBLIC EIGEN_USE_BLAS)
endif()
if(PUMILA_SDL2)
    target_link_directories(pumila PRIVATE ${SDL2_LIBDIR} ${SDL2TTF_LIBDIR})
    target_link_libraries(pumila PRIVATE ${SDL2_LIBRARIES} ${SDL2TTF_LIBRARIES})
    target_compile_definitions(pumila PRIVATE PUMILA_SDL2)
endif()

pybind11_add_module(pypumila
    src/pypumila/pypumila.cc
    src/pypumila/pypumila1.cc
    src/pypumila/pypumila2.cc
    src/pypumila/pypumila3.cc
)
target_link_libraries(pypumila PUBLIC pumila)

if(PUMILA_SDL2)
    add_executable(pumila-play src/play/main.cc)
    target_link_libraries(pumila-play PRIVATE pumila)
endif()
