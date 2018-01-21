cmake_minimum_required (VERSION 3.0)
project (sfml-box2d-demo)

set(CMAKE_CXX_STANDARD 14)

include_directories(extlibs/sfml/include)
include_directories(extlibs/thor/include)
include_directories(extlibs/thor/extlibs/aurora/include)
include_directories(extlibs/box2d/Box2D)

# build extlibs
add_subdirectory(extlibs)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
add_executable(demo src/main.cpp)
target_link_libraries(demo sfml-system sfml-graphics sfml-window sfml-audio thor box2d)

