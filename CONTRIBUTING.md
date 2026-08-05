# Contributing

You want to contribute? Awesome!

1. Fork
2. cover your change with tests
3. Format the code with `clang-format`
4. create a PR

## Fuzzing

`data/fuzz/api` is a minimized corpus that the normal test suite replays on every run. That
guards against regressions but never finds anything new, so a nightly job does the finding. It
can also be started by hand from the Actions tab, with a duration.

To fuzz locally, from a clang build directory:

```sh
CXX=clang++ meson setup builddir
cd builddir
../scripts/fuzz_run.sh api    # accumulates into CORPUS_BIG/
../scripts/fuzz_merge.sh api  # folds back only what adds coverage
```

**New corpus entries are committed by hand, never by CI.** The nightly job uploads what it finds
as a `corpus-new` artifact and stops there, because a job that pushes to the repository needs
write access it has no other reason to have. Download the artifact, drop the files into
`data/fuzz/api`, check the suite still passes, and commit them.

A crash fails the job and uploads the reproducer as a `fuzz-crash` artifact. Reproduce it with
`./builddir/test/fuzz_api <file>`. Treat it as a real bug: most of the defects this container
has had were exception safety and aliasing problems found this way rather than reported.

## Developer Certificate of Origin (DCO)

All contributions (including pull requests) must agree to the [Developer Certificate of Origin (DCO) version 1.1](https://developercertificate.org). This is a developer's certification that he or she has the right to submit the patch for inclusion into the project.

Simply submitting a contribution implies this agreement, however, please include a `Signed-off-by` tag in the PR (this tag is a conventional way to confirm that you agree to the DCO).


## Governance

This project uses the BDFL (Benevolent Dictator For Life) model. All decisions are made by me, Martin Leitner-Ankerl. Wohoo!


## Roles & Responsibilites

### User

SOneone who uses or has used `svector`.

### Contributor

Someone who has helped the `svector` project, who has contributed to bring it forward. Contributing could be to provide advice, debug a problem, file a bug report, run test infrastructure or writing code etc.

### BDFL (Benevolent Dictator For Life)

That's Martin Leitner-Ankerl.
