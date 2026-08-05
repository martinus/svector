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
    'unit/allocator.cpp',
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
    'unit/insert_exception_safety.cpp',
    'unit/member_types.cpp',
    'unit/move_relocation.cpp',
    'unit/n_eq_0.cpp',
    'unit/noexcept_contract.cpp',
    'unit/pop_back.cpp',
    'unit/push_back.cpp',
    'unit/reserve.cpp',
    'unit/resize.cpp',
    'unit/resize_and_overwrite.cpp',
    'unit/reverse_iterators.cpp',
    'unit/round_up.cpp',
    'unit/self_referencing_insert.cpp',
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

# fmt is only used by the test harness. A system fmt is fine normally, but it is built for the
# host word size, so a -m32 build cannot link against it and has to build fmt from the wrap.
#
# method: 'builtin' used to say that and silently stopped meaning it: restricting the method
# keeps meson from consulting the wrap's override_dependency, so the fallback came back NO and
# a 32 bit setup failed outright rather than falling back. Ask for the subproject directly.
if get_option('cpp_args').contains('-m32')
    fmt_dep = subproject('fmt').get_variable('fmt_dep')
else
    fmt_dep = dependency('fmt')
endif

# boost and abseil are only used to compare against in the benchmarks and in
# show_comparison, never by the actual tests. Both are guarded by __has_include in
# app/boost_absl.h, so everything builds fine without them and the comparison comes back
# by itself once the headers are installed system wide.
#
# Neither is fetched here any more. abseil had a wrap, but a wrap is only consulted as a
# fallback and meson does not fall back for an optional dependency, so absl_container came
# up NO on every CI run anyway. boost was resolving, but pulling it in cost the linux job
# up to 59s of apt for code that meson test never executes.

test_exe = executable(
    'test-svector',
    test_sources,
    include_directories: incdir,
    cpp_args: cpp_args,
    dependencies: [
        dependency('threads'), # add dependency for threads (-lpthread, see https://mesonbuild.com/howtox.html),

        # see what's in the [provide] sections for the dependency names
        dependency('doctest'),
        fmt_dep,
    ],
)

benchmark(
    'bench',
    test_exe,
    args: ['-ns', '-ts=bench'],
    verbose: true)

# One workload against one container per process, which is the only way to compare four allocating
# containers without the allocator's warm-up state deciding the winner. See test/bench/solo.cpp.
# Not registered as a benchmark(): it takes arguments and is driven by scripts/bench/render_charts.py.
executable(
    'bench-solo',
    ['bench/solo.cpp', 'app/nanobench.cpp'],
    include_directories: incdir,
    cpp_args: cpp_args,
    dependencies: [dependency('threads')],
)

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
            fmt_dep,
        ]
    fuzz_cpp_args = [
        '-DFUZZ',
        '-fsanitize-undefined-trap-on-error',
        '-fsanitize=undefined,address,fuzzer',
        '-g',
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
