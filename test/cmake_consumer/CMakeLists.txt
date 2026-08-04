cmake_minimum_required(VERSION 3.12)
project("svector-cmake-consumer" CXX)

# Consume svector the way a downstream project does. This is the only CMake integration
# path the repository supports, and nothing else in CI builds CMakeLists.txt at all, so a
# typo in the interface target's include directories would otherwise ship unnoticed.
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../.." svector)

add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE svector::svector)
