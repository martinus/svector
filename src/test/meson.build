test_sources = [
    'AlphabetMapTest.cpp',
    'IsSingleWordValidTest.cpp',
    'main.cpp',
    'parseDictTest.cpp',
    'stateFromWordTest.cpp',
]

wordle_data_dir = '-DWORDLE_DATA_DIR="@0@"'.format(meson.global_source_root())
test_exe = executable(
    'test-wordle',
    test_sources,
    include_directories: lib_inc,
    dependencies: thread_dep,
    cpp_args: wordle_data_dir,
    link_with: wordle_lib
)

test(
    'wordle test',
    test_exe,
    # verbose: true # only meson 0.62
)

