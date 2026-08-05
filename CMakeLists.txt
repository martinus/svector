cmake_minimum_required(VERSION 3.12)
project("svector"
    VERSION 1.4.0
    DESCRIPTION " Compact SVO optimized vector for C++17 or higher"
    HOMEPAGE_URL "https://github.com/martinus/svector")

include(GNUInstallDirs)

# PROJECT_IS_TOP_LEVEL only arrived in CMake 3.21 and the floor here is 3.12, so this is spelled
# by hand.
set(_svector_is_top_level OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(_svector_is_top_level ON)
endif()

# Defined unconditionally. It used to exist only when svector was pulled in with
# add_subdirectory, which meant configuring this repository on its own produced nothing at all --
# see issue #48, where exactly that looked like the project was broken. An install also needs a
# target to export.
add_library(svector INTERFACE)
add_library(svector::svector ALIAS svector)
target_compile_features(svector INTERFACE cxx_std_17)
target_include_directories(
    svector
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# CMake still never builds or runs the tests, that is meson's job. What it does now is install,
# so that find_package(svector) works and packagers have something to call.
#
# Only when svector is the project being built: a parent that pulled it in with add_subdirectory
# should not find svector's headers turning up in its own install.
if(_svector_is_top_level)
    include(CMakePackageConfigHelpers)

    set(_svector_cmakedir "${CMAKE_INSTALL_LIBDIR}/cmake/svector")

    install(TARGETS svector EXPORT svectorTargets)
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
    install(EXPORT svectorTargets
        FILE svectorTargets.cmake
        NAMESPACE svector::
        DESTINATION "${_svector_cmakedir}")

    # ARCH_INDEPENDENT stops the version file rejecting a package whose word size differs from
    # the consumer's, which for a header only library is not a real incompatibility. It needs
    # CMake 3.14, above the floor, so it is added only where it exists.
    set(_svector_version_args)
    if(NOT CMAKE_VERSION VERSION_LESS 3.14)
        list(APPEND _svector_version_args ARCH_INDEPENDENT)
    endif()

    # The version comes from project() above, which scripts/lint/lint-version.py keeps in step
    # with the ANKERL_SVECTOR_VERSION_* macros, so a package can never claim a version the
    # header does not. SameMajorVersion is the compatibility the header's own semver comment
    # promises.
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/svectorConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
        ${_svector_version_args})

    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/svectorConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/svectorConfig.cmake"
        INSTALL_DESTINATION "${_svector_cmakedir}")

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/svectorConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/svectorConfigVersion.cmake"
        DESTINATION "${_svector_cmakedir}")

    # For the consumers that never touch CMake at all, which is most of what a distribution
    # packages.
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/svector.pc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/svector.pc"
        @ONLY)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/svector.pc"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig")
endif()
