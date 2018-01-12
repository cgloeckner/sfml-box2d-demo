cmake_minimum_required (VERSION 3.0)
project (sfml-box2d-demo)

add_subdirectory(sfml)

# build Box2D
file(GLOB_RECURSE box2d_src "box2d/Box2D/Box2D/*.cpp")
add_library(box2d ${box2d_src})

