test_sources = [
    'app/Counter.cpp',
    'app/doctest.cpp',
    'app/nanobench.cpp',

    'bench/bench_accumulate.cpp',
    'bench/bench_emplace_back.cpp',
    'bench/bench_fill_front.cpp',
    'bench/bench_fill_random.cpp',
    'bench/bench_push_pop.cpp',
    'bench/bench_sort_shuffle.cpp',

    'assign.cpp',
    'assignment_operators.cpp',
    'at.cpp',
    'automatic_capacity.cpp',
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
    'n_eq_0.cpp',
    'pop_back.cpp',
    'push_back.cpp',
    'reserve.cpp',
    'resize.cpp',
    'reverse_iterators.cpp',
    'round_up.cpp',
    'show_comparison.cpp',
    'shrink_to_fit.cpp',
    'std_erase.cpp',
    'swap.cpp',
]

# additional compile options
cpp_args = ''
compiler = meson.get_compiler('cpp')
if compiler.has_argument('-Wno-stringop-overflow')
    cpp_args += '-Wno-stringop-overflow' # g++ error in fmtlib
endif

fmt_method = 'auto'
if get_option('cpp_args').contains('-m32')
    # use builtin so we can compile it for 32bit. 
    # Can't use it as a default or sanitizer doesn't work...
    fmt_method = 'builtin'
endif

test_exe = executable(
    'test-svector',
    test_sources,
    include_directories: lib_inc,
    cpp_args: cpp_args,
    dependencies: [
        dependency('threads'), # add dependency for threads (-lpthread, see https://mesonbuild.com/howtox.html),

        # see what's in the [provide] sections for the dependency names
        dependency('absl_container', default_options: ['warning_level=0', 'werror=false']), # just for comparison with inline_vector
        dependency('boost'),
        dependency('doctest'),
        dependency('fmt', method: fmt_method),
    ],
)

benchmark(
    'bench',
    test_exe,
    args: ['-ns', '-ts=bench'],
    verbose: true)

test(
    'svector test',
    test_exe,
    verbose: true)

