cmake_minimum_required(VERSION 3.0)
project(pumila CXX)
find_package(Eigen3 REQUIRED)
find_package(SDL2)

add_library(pumila SHARED
    src/pumila/game.cc
    src/pumila/chain.cc
    src/pumila/window.cc
)
target_include_directories(pumila PUBLIC include)
target_compile_features(pumila PUBLIC cxx_std_11)
target_link_libraries(pumila PRIVATE ${SDL2_LIBRARIES})
target_compile_definitions(pumila PRIVATE
    PUMILA_SDL2=${SDL2_FOUND}
)

add_executable(pumila-play src/play/main.cc)
target_link_libraries(pumila-play PRIVATE pumila)
