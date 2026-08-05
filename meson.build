# meson commands cheat sheet:
#
# # Setup ###################
#
# release & debug setup
#      CXX="ccache clang++" meson setup --buildtype release build/clang_release
#      CXX="ccache clang++" meson setup build/clang_debug
# c++20 build
#     CXX="ccache clang++" meson setup -Dcpp_std=c++20 build/clang_cpp20
# lcov coverage:
#     CXX="ccache clang++" meson setup -Db_coverage=true build/coverage
#     ninja clean && ninja test && ninja coverage
#
# # Testing ################
#
# Run with valgrind:
#     meson test --wrap='valgrind --leak-check=full --error-exitcode=1'
#

project('svector', 'cpp',
    version: '1.3.0',
    license: 'MIT',
    default_options : [
        'cpp_std=c++17',
        'warning_level=3',
        'werror=true',
        'b_ndebug=true', # otherwise absl is really slow!
    ])

incdir = include_directories('include')

# So a parent project can use svector as a meson subproject: with this, its dependency('svector')
# resolves to us through the wrap. There was no way to consume this from meson at all before.
svector_dep = declare_dependency(include_directories: incdir)
meson.override_dependency('svector', svector_dep)

# A parent pulling svector in wants the header, not our test suite and not our headers in its
# install tree.
if not meson.is_subproject()
    install_headers('include/ankerl/svector.h', subdir: 'ankerl')
    subdir('test')
endif

