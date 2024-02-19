cmake_minimum_required(VERSION 3.18)
project(pumila CXX)
if(NOT DEFINED CMAKE_BUILD_TYPE)
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

option(PUMILA_MKL off "Use MKL (fot Intel CPU on Linux / Windows)")
set(PUMILA_MKL_ROOT "C:/Program Files (x86)/Intel/oneAPI" CACHE STRING "oneMKL Root Path (for Windows)")
option(PUMILA_ACCELERATE off "Use Accelerate (for MacOS)")

if(NOT MSVC)
    include(FindPkgConfig)
endif()
include(FetchContent)

find_package(Eigen3 QUIET)
if(Eigen3_FOUND)
    message(STATUS "Eigen3 found")
    set(eigen Eigen3::Eigen)
else()
    message(STATUS "Eigen3 not found, fetching source...")
    FetchContent_Declare(eigen
        GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
        GIT_TAG 3.4.0
    )
    FetchContent_Populate(eigen)
    add_library(eigen INTERFACE)
    target_include_directories(eigen INTERFACE ${eigen_SOURCE_DIR})
    set(eigen eigen)
endif()

if(MSVC)
    message(STATUS "Fetching SDL2 and SDL2_ttf ...")
    FetchContent_Declare(sdl2
        URL https://github.com/libsdl-org/SDL/releases/download/release-2.30.0/SDL2-devel-2.30.0-VC.zip
    )
    FetchContent_Declare(sdl2_ttf
        URL https://github.com/libsdl-org/SDL_ttf/releases/download/release-2.22.0/SDL2_ttf-devel-2.22.0-VC.zip
    )
    FetchContent_Populate(sdl2)
    FetchContent_Populate(sdl2_ttf)
    set(sdl2_DIR ${sdl2_SOURCE_DIR}/cmake)
    set(sdl2_ttf_DIR ${sdl2_ttf_SOURCE_DIR}/cmake)
    find_package(sdl2 REQUIRED)
    find_package(sdl2_ttf REQUIRED)
    set(PUMILA_SDL2 1)
    set(sdl2 SDL2::SDL2 SDL2_ttf::SDL2_ttf)
    configure_file(${sdl2_SOURCE_DIR}/lib/x64/SDL2.dll SDL2.dll COPYONLY)
    configure_file(${sdl2_ttf_SOURCE_DIR}/lib/x64/SDL2_ttf.dll SDL2_ttf.dll COPYONLY)
else()
    pkg_search_module(sdl2 sdl2)
    pkg_search_module(sdl2_ttf SDL2_ttf>=2.0)
    if(sdl2_FOUND AND sdl2_ttf_FOUND)
        set(PUMILA_SDL2 1)
        message(STATUS "sdl2 and SDL2_ttf found")
        add_library(sdl2 INTERFACE)
        target_link_directories(sdl2 INTERFACE
            ${sdl2_LIBDIR} ${sdl2_ttf_LIBDIR}
        )
        target_link_libraries(sdl2 INTERFACE
            ${sdl2_LIBRARIES} ${sdl2_ttf_LIBRARIES}
        )
        target_include_directories(sdl2 INTERFACE
            ${sdl2_INCLUDE_DIRS} ${sdl2_ttf_INCLUDE_DIRS}
        )
        set(sdl2 sdl2)
    else()
        set(PUMILA_SDL2 0)
        message(STATUS "sdl2 and SDL2_ttf not found (SDL2_FOUND = '${SDL2_FOUND}', SDL2TTF_FOUND = '${SDL2TTF_FOUND}')")
    endif()
endif()

if(PUMILA_MKL)
    if(MSVC)
        add_library(mkl INTERFACE)
        target_link_libraries(mkl INTERFACE mkl_rt.lib)
        target_link_directories(mkl INTERFACE
            ${PUMILA_MKL_ROOT}/mkl/latest/lib
        )
        target_include_directories(mkl INTERFACE
            ${PUMILA_MKL_ROOT}/mkl/latest/include
        )
        set(MKL_DLLS mkl_rt.2.dll mkl_intel_thread.2.dll)
        foreach(dll ${MKL_DLLS})
            configure_file(
                ${PUMILA_MKL_ROOT}/mkl/latest/bin/${dll}
                ${dll}
                COPYONLY
            )
        endforeach()
        message(STATUS "mkl libraries found")
        set(mkl mkl)
    else()
        pkg_search_module(MKL REQUIRED mkl-dynamic-lp64-iomp)
        message(STATUS "mkl-dynamic-lp64-iomp found")
        add_library(mkl INTERFACE)
        target_link_directories(mkl INTERFACE ${MKL_LIBDIR})
        target_include_directories(mkl INTERFACE ${MKL_INCLUDE_DIRS})
        target_link_libraries(mkl INTERFACE ${MKL_LIBRARIES} ${MKL_LDFLAGS})
        # target_link_options(mkl INTERFACE ${MKL_LDFLAGS})
        set(mkl mkl)
    endif()
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

if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wpedantic -Wextra)
    add_compile_options(-UNDEBUG)
endif()

set(PUMILA_SRC
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
    src/pumila/models/pumila4.cc
    src/pumila/models/pumila5.cc
    src/pumila/models/pumila6.cc
    src/pumila/models/pumila7.cc
    src/pumila/models/pumila8.cc
    src/pumila/models/pumila6r.cc
    src/pumila/models/pumila7r.cc
    src/pumila/models/pumila8r.cc
)
if(WIN32)
    set(PUMILA_SRC ${PUMILA_SRC} src/pumila/version.rc)
endif()
add_library(pumila SHARED ${PUMILA_SRC})
target_include_directories(pumila PUBLIC include)
target_compile_features(pumila PUBLIC cxx_std_20)
target_link_libraries(pumila PUBLIC ${eigen} bs_thread_pool)
if(PUMILA_MKL)
    target_link_libraries(pumila PUBLIC ${mkl})
    target_compile_definitions(pumila PUBLIC EIGEN_USE_MKL_ALL)
endif()
if(PUMILA_ACCELERATE)
    target_link_libraries(pumila PUBLIC ${ACCELERATE})
    target_compile_definitions(pumila PUBLIC EIGEN_USE_BLAS)
endif()
if(PUMILA_SDL2)
    target_link_libraries(pumila PRIVATE ${sdl2})
    target_compile_definitions(pumila PRIVATE PUMILA_SDL2)
endif()
if(MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX d)
endif()

pybind11_add_module(pypumila
    src/pypumila/pypumila.cc
    src/pypumila/pypumila1.cc
    src/pypumila/pypumila2.cc
    src/pypumila/pypumila3.cc
    src/pypumila/pypumila4.cc
    src/pypumila/pypumila5.cc
    src/pypumila/pypumila6.cc
    src/pypumila/pypumila7.cc
    src/pypumila/pypumila8.cc
    src/pypumila/pypumila6r.cc
    src/pypumila/pypumila7r.cc
    src/pypumila/pypumila8r.cc
)
target_link_libraries(pypumila PUBLIC pumila)

if(PUMILA_SDL2)
    add_executable(pumila-sim src/sim/main.cc)
    target_link_libraries(pumila-sim PRIVATE
        pumila
        CLI11
    )
endif()
