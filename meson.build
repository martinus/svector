# compile with
#
# CXX="ccache clang++" meson setup --buildtype release build/clang_release
# CXX="ccache clang++" meson setup build/clang_debug
# meson setup --buildtype release build/g++_release
# meson setup build/g++_debug

project('svector', 'cpp',
    version: '1.0.0',
    license: 'MIT',
    default_options : ['cpp_std=c++17', 'warning_level=3', 'werror=true'])

subdir('src/lib')
subdir('src/test')
