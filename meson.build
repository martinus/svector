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
    version: '1.0.3',
    license: 'MIT',
    default_options : [
        'cpp_std=c++17',
        'warning_level=3',
        'werror=true',
        'b_ndebug=true', # otherwise absl is really slow!
    ])

incdir = include_directories('include')
subdir('test')

