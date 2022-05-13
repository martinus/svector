test_sources = [
    'app/doctest.cpp',
    'app/nanobench.cpp',
    'basic_api.cpp',
]

test_exe = executable(
    'test-svector',
    test_sources,
    include_directories: lib_inc,
    dependencies: [
        dependency('threads'), # add dependency for threads (-lpthread, see https://mesonbuild.com/howtox.html),

        # see what's in the [provide] sections for the dependency names
        dependency('doctest'),
        dependency('fmt'),
        dependency('absl_container'), # just for comparison with inline_vector
        dependency('boost')
    ]
)

test(
    'svector test',
    test_exe,
    verbose: false # only meson 0.62
)

