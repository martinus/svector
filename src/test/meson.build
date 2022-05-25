test_sources = [
    'app/Counter.cpp',
    'app/doctest.cpp',
    'app/nanobench.cpp',
    'bench/bench_push_pop.cpp',

    'assign.cpp',
    'assignment_operators.cpp',
    'at.cpp',
    'bad_alloc.cpp',
    'basic_api.cpp',
    'comparisons.cpp',
    'ctors.cpp',
    'emplace_back.cpp',
    'emplace.cpp',
    'erase.cpp',
    'includes_only.cpp',
    'insert.cpp',
    'member_types.cpp',
    'pop_back.cpp',
    'push_back.cpp',
    'reserve.cpp',
    'resize.cpp',
    'reverse_iterators.cpp',
    'show_comparison.cpp',
    'shrink_to_fit.cpp',
    'swap.cpp',
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

