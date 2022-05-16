test_sources = [
    'app/Counter.cpp',
    'app/doctest.cpp',
    'app/nanobench.cpp',
    'bench/bench_push_pop.cpp',

    'assign.cpp',
    'basic_api.cpp',
    'ctors.cpp',
    'emplace_back.cpp',
    'includes_only.cpp',
    'member_types.cpp',
    'push_back.cpp',
    'show_comparison.cpp',
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
        dependency('absl_container', default_options: ['warning_level=0', 'werror=false']), # just for comparison with inline_vector
        dependency('boost')
    ]
)

test(
    'svector test',
    test_exe,
    verbose: true # only meson 0.62
)

