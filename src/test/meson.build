test_sources = [
    'hello_world.cpp',
    'main.cpp',
]

test_exe = executable(
    'test-svector',
    test_sources,
    include_directories: lib_inc,
    dependencies: thread_dep,
)

test(
    'wordle test',
    test_exe,
    # verbose: true # only meson 0.62
)

