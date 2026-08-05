#!/usr/bin/env python3

"""Run clang-tidy over the header, at a pinned version.

The version is pinned on purpose. .clang-tidy enables whole families, and families gain checks in
every clang-tidy release, so an unpinned version would turn a toolchain bump into a red build for
reasons that have nothing to do with a change. That is exactly why the config sat in this
repository for years without ever being enforced.

Two ways to get that version, tried in order: a clang-tidy-N binary on PATH, or a container image.
CI installs the binary, which keeps the lint job quick and off Docker Hub, whose anonymous pull
limit is shared across GitHub's runners and would make a required check flaky. The image is for
contributors who would rather not install a second clang.

svector is a template, so a check only sees what something instantiated. The translation unit
below therefore exercises a broad spread of the API: with only a push_back, most of the container
is never looked at.
"""

import os
import pathlib
import shutil
import subprocess
import sys
import tempfile


PINNED_MAJOR = 18
BINARY = f"clang-tidy-{PINNED_MAJOR}"
IMAGE = f"docker.io/silkeh/clang:{PINNED_MAJOR}"

# Instantiates enough of the template that clang-tidy actually looks at it: both storage modes,
# both element kinds that matter (trivially copyable and not), and the operations with the
# interesting implementations behind them.
TU = """
#include <ankerl/svector.h>

#include <string>
#include <utility>

template <typename T>
void exercise(T const& value) {
    auto a = ankerl::svector<T, 7>();
    auto b = ankerl::svector<T, 7>();

    for (int i = 0; i < 100; ++i) {
        a.push_back(value);
        a.emplace_back(value);
    }
    a.insert(a.begin(), value);
    a.insert(a.begin(), 5, value);
    a.insert(a.end(), b.begin(), b.end());
    a.emplace(a.begin(), value);
    a.erase(a.begin());
    a.erase(a.begin(), a.begin() + 2);
    a.resize(200, value);
    a.resize(2);
    a.assign(7, value);
    a.reserve(500);
    a.shrink_to_fit();
    a.pop_back();

    b = a;
    b = std::move(a);
    auto c = b;
    auto d = std::move(c);
    swap(b, d);
    b.swap(d);

    (void)b.at(0);
    (void)b[0];
    (void)b.front();
    (void)b.back();
    (void)b.data();
    (void)b.size();
    (void)b.capacity();
    (void)b.empty();
    (void)(b == d);
    (void)(b < d);
    b.clear();
}

auto main() -> int {
    try {
        exercise<int>(42);
        exercise<std::string>("a string long enough that it really allocates");
    } catch (...) {
        return 1;
    }
    return 0;
}
"""


def container_runtime():
    # Being on PATH is not enough: a docker binary with no daemon behind it is common, and it
    # would fail at the point of use rather than here.
    for candidate in ("docker", "podman"):
        if not shutil.which(candidate):
            continue
        if subprocess.run([candidate, "info"], capture_output=True).returncode == 0:
            return candidate
    return None


def clang_tidy_command(root):
    """How to invoke the pinned clang-tidy, or None if it cannot be found at all.

    The container comes first because it pins both halves of the problem. Pinning only the
    compiler is not really a pin: clang-tidy parses the code with whatever standard library the
    machine has, and a libstdc++ newer than the pinned clang does not compile. On Fedora 44,
    clang-tidy-18 against libstdc++ 16 dies inside <string>.

    SVECTOR_CLANG_TIDY overrides that for an environment known to be consistent. CI sets it, so
    the lint job stays quick and does not depend on Docker Hub, whose anonymous pull limit is
    shared across GitHub's runners and would make a required check flaky.
    """
    override = os.environ.get("SVECTOR_CLANG_TIDY")
    if override:
        return [override], override

    runtime = container_runtime()
    if runtime is not None:
        return [runtime, "run", "--rm", "-v", f"{root}:/src:z", "-w", "/src", IMAGE, "clang-tidy"], IMAGE

    if shutil.which(BINARY):
        return [BINARY], BINARY

    return None, None


def main() -> int:
    root = os.path.abspath(pathlib.Path(__file__).parent.parent.parent)

    command, where = clang_tidy_command(root)
    if command is None:
        print(f"SKIPPED clang-tidy: neither {BINARY} nor a working docker or podman is available")
        return 0

    # Written inside the repository so clang-tidy finds .clang-tidy by walking up from the file,
    # the same way it does for anyone running it by hand.
    with tempfile.NamedTemporaryFile(mode="w", suffix=".cpp", dir=root, delete=False) as f:
        generated = f.name
        f.write(TU)

    try:
        result = subprocess.run(
            command + [os.path.basename(generated), "--quiet", "--", "-std=c++17", "-I", "include"],
            capture_output=True,
            text=True,
            cwd=root,
        )
    finally:
        os.unlink(generated)

    if result.returncode != 0:
        # The generated name is a temporary, so it is noise in a diagnostic.
        print(result.stdout.replace(os.path.basename(generated), "<generated>"), end="")
        print(result.stderr, end="", file=sys.stderr)
        return 1

    print(f"clang-tidy ({where}) checked include/ankerl/svector.h")
    return 0


if __name__ == "__main__":
    sys.exit(main())
