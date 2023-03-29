test_sources = [
    'app/Counter.cpp',
    'app/doctest.cpp',
    'app/nanobench.cpp',
    'app/ui/Periodic.cpp',
    'app/ui/ProgressBar.cpp',
    'bench/bench_accumulate.cpp',
    'bench/bench_emplace_back.cpp',
    'bench/bench_fill_front.cpp',
    'bench/bench_fill_random.cpp',
    'bench/bench_push_back.cpp',
    'bench/bench_randomaccess.cpp',
    'bench/bench_sort_shuffle.cpp',
    'fuzz/api.cpp',
    'unit/alignment.cpp',
    'unit/assign.cpp',
    'unit/assignment_operators.cpp',
    'unit/at.cpp',
    'unit/automatic_capacity.cpp',
    'unit/bad_alloc.cpp',
    'unit/basic_api.cpp',
    'unit/comparisons.cpp',
    'unit/ctors.cpp',
    'unit/emplace_back.cpp',
    'unit/emplace.cpp',
    'unit/erase.cpp',
    'unit/fuzz_corpus.cpp',
    'unit/includes_only.cpp',
    'unit/insert.cpp',
    'unit/member_types.cpp',
    'unit/n_eq_0.cpp',
    'unit/non_movable.cpp',
    'unit/pop_back.cpp',
    'unit/push_back.cpp',
    'unit/reserve.cpp',
    'unit/resize.cpp',
    'unit/reverse_iterators.cpp',
    'unit/round_up.cpp',
    'unit/show_comparison.cpp',
    'unit/shrink_to_fit.cpp',
    'unit/std_erase.cpp',
    'unit/swap.cpp',
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

opt_boost = dependency('boost', required: false)
opt_absl = dependency('absl_container', default_options: ['warning_level=0', 'werror=false'], required: false)

test_exe = executable(
    'test-svector',
    test_sources,
    include_directories: incdir,
    cpp_args: cpp_args,
    dependencies: [
        dependency('threads'), # add dependency for threads (-lpthread, see https://mesonbuild.com/howtox.html),

        # see what's in the [provide] sections for the dependency names
        opt_absl,
        opt_boost,
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

test(
    'fuzz',
    test_exe,
    args: ['-ns', '-ts=fuzz'],
    env: 'FUZZ_CORPUS_BASE_DIR=' + meson.source_root() / 'data' / 'fuzz',
    verbose: true)



if compiler.get_id() == 'clang'
    fuzz_deps = [
            dependency('doctest'),
            dependency('threads'),
            dependency('fmt', method: fmt_method),
        ]
    fuzz_cpp_args = [
        '-DFUZZ',
        '-fsanitize-undefined-trap-on-error',
        '-fsanitize=undefined,address,fuzzer',
        '-g',
        '-isystem', '/usr/lib64/clang/14.0.0/include/',
    ]
    fuzz_link_args = ['-fsanitize=undefined,address,fuzzer']
    fuzz_sources = [
        'app/Counter.cpp',
    ]

    executable(
        'fuzz_api',
        fuzz_sources + ['fuzz/api.cpp'],
        include_directories: incdir,
        cpp_args : fuzz_cpp_args,
        link_args: fuzz_link_args,
        dependencies: fuzz_deps,
    )
endif
