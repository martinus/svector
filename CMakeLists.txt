cmake_minimum_required(VERSION 3.12)
project("svector"
    VERSION 1.0.0
    DESCRIPTION " Compact SVO optimized vector for C++17 or higher"
    HOMEPAGE_URL "https://github.com/martinus/svector")

# determine whether this is a standalone project or included by other projects
set(_svector_is_included ON)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(_svector_is_included OFF)
endif()

if(_svector_is_included)
    add_library(svector INTERFACE)
    add_library(svector::svector ALIAS svector)
    target_compile_features(svector INTERFACE cxx_std_17)
    target_include_directories(svector PUBLIC ${PROJECT_SOURCE_DIR}/include)
endif()
