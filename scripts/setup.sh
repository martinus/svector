#!/bin/bash

# Creates multiple meson setups for ease of use
set -e
cd `git rev-parse --show-toplevel`

CXX="ccache clang++" meson setup --buildtype release  -Dcpp_std=c++17 build/clang_cpp17_release
CXX="ccache clang++" meson setup --buildtype debug    -Dcpp_std=c++17 build/clang_cpp17_debug
CXX="ccache g++"     meson setup --buildtype release  -Dcpp_std=c++17 build/gcc_cpp17_release
CXX="ccache g++"     meson setup --buildtype debug    -Dcpp_std=c++17 build/gcc_cpp17_debug

# 32bit. Install lib32-clang
CXX="ccache g++"     meson setup --buildtype debug    -Dcpp_std=c++17 -Dcpp_args='-m32' -Dcpp_link_args='-m32' -Dc_args='-m32' -Dc_link_args='-m32' build/gcc_cpp17_debug_32
CXX="ccache clang++" meson setup --buildtype debug    -Dcpp_std=c++17 -Dcpp_args='-m32' -Dcpp_link_args='-m32' -Dc_args='-m32' -Dc_link_args='-m32' build/clang_cpp17_debug_32

# c++20
CXX="ccache clang++" meson setup --buildtype debug    -Dcpp_std=c++20 build/clang_cpp20_debug
CXX="ccache g++"     meson setup --buildtype debug    -Dcpp_std=c++20 build/gcc_cpp20_debug

# coverage; use "ninja clean && ninja test && ninja coverage"
CXX="ccache clang++" meson setup -Db_coverage=true                    build/coverage

# sanitizers
# It is not possible to combine more than one of the -fsanitize=address, -fsanitize=thread, and -fsanitize=memory checkers in the same program.
# see https://clang.llvm.org/docs/UsersManual.html#controlling-code-generation
#
# can't use ccache, it doesn't work with the ignorelist.txt
CXX="ccache clang++" meson setup -Db_sanitize=address                 build/clang_sanitize_address
CXX="ccache clang++" meson setup -Db_sanitize=thread                  build/clang_sanitize_thread
# CXX="ccache clang++" meson setup -Db_sanitize=memory                  build/clang_sanitize_memory # doesn't work due to STL, and ignore doesn't work either :-(
CXX="ccache clang++" meson setup -Db_sanitize=undefined               build/clang_sanitize_undefined
