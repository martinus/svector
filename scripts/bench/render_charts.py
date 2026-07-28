#!/usr/bin/env python3
"""Measure every container in its own process and render the charts the README shows.

Four containers benchmarked one after the other inside a single process do not measure the same
thing: glibc adapts its mmap threshold the first time a large block is freed, so whichever one runs
first pays for mmap and munmap and trains the allocator for the rest. On an emplace_back of 10000
std::string that is worth a factor of three, and it follows position in the list, not the container.
test/bench/solo.cpp exists to avoid that -- one workload, one container, one process. This drives it
and takes the median over several runs.

    meson setup builddir --buildtype=release -Dcpp_args="-isystem /path/to/abseil-cpp"
    meson compile -C builddir
    ./scripts/bench/render_charts.py builddir/test/bench-solo doc

The comparison against absl and boost only happens if their headers were visible at compile time,
see test/app/boost_absl.h.
"""
import argparse
import os
import re
import statistics
import subprocess
import sys

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

# nanobench prints one markdown row per benchmark; solo runs produce exactly one.
ROW = re.compile(
    r"^\|\s*([\d,\.]+)\s*\|\s*([\d,\.]+)\s*\|\s*([\d\.]+)%\s*\|\s*([\d,\.]+)\s*\|\s*([\d,\.]+)"
    r"\s*\|\s*([\d\.]+)\s*\|.*?`([^`]+)`(.*)$"
)

BLUE = "#4285f4"
RED = "#ea4335"
GREY = "#5f6368"
LIGHT = "#dadce0"

CONTAINERS = [
    (0, "std::vector"),
    (1, "absl::InlinedVector"),
    (2, "boost::container::\nsmall_vector"),
    (3, "ankerl::svector"),
]

# workload, png, title, subtitle, x label, scale applied to ns
CHARTS = [
    ("push_back", "bench_push_back.png", "push_back",
     "push_back 1000 uint8_t onto a fresh vector, inline capacity 7",
     "ns per push_back  (less is better)", 1.0),
    ("emplace_back_string", "bench_emplace_back_string.png", "emplace_back",
     'emplace_back 10000 std::string("hello") onto a fresh vector, inline capacity 7',
     "ns per emplace_back  (less is better)", 1.0),
    ("random_access", "bench_randomaccess.png", "random access",
     "sum 1000 int read through operator[] at random indices, inline capacity 7",
     "ns per read  (less is better)", 1.0),
    ("insert_random_int", "bench_random_insert.png", "insert at a random position",
     "grow to 1000 uint64_t, each emplace()d at a random index, inline capacity 7",
     "ns per insert  (less is better)", 1.0),
    ("insert_random_string", "bench_random_insert_string.png", "insert at a random position",
     "grow to 1000 std::string, each emplace()d at a random index, inline capacity 7",
     "ns per insert  (less is better)", 1.0),
    ("shuffle_sort", "bench_shuffle_sort.png", "shuffle + sort",
     "shuffle and then std::sort 1000 std::string, inline capacity 7",
     "µs per shuffle+sort  (less is better)", 1 / 1000.0),
]

# measured but not charted, they go in the README's table
EXTRA = ["accumulate", "insert_front_int", "insert_front_string"]


def measure(binary, workload, container, runs, cpu):
    """Median ns and instruction count over `runs` fresh processes. None if unavailable."""
    samples = []
    ins = []
    for _ in range(runs):
        cmd = [binary, workload, str(container)]
        if cpu is not None:
            cmd = ["taskset", "-c", cpu] + cmd
        done = subprocess.run(cmd, capture_output=True, text=True)
        if done.returncode == 2:  # built without this container
            return None
        if done.returncode != 0:
            sys.stderr.write(done.stdout + done.stderr)
            sys.exit("%s %s failed" % (workload, container))
        for line in done.stdout.splitlines():
            m = ROW.match(line)
            if m:
                samples.append(float(m.group(1).replace(",", "")))
                ins.append(float(m.group(4).replace(",", "")))
    if not samples:
        sys.exit("no measurement parsed for %s %s" % (workload, container))
    return {
        "ns": statistics.median(samples),
        "ins": statistics.median(ins),
        # the first run of a process family is often cold, so report the spread of the rest
        "spread": (max(samples[1:]) / min(samples[1:]) - 1) if len(samples) > 2 else 0.0,
    }


def render(path, title, subtitle, xlabel, rows):
    if not rows:
        print("skipping %s, nothing to plot" % path)
        return

    fig_h = 1.45 + 0.62 * len(rows)
    fig, ax = plt.subplots(figsize=(7.2, fig_h), dpi=140)

    values = [r[1] for r in rows]
    colors = [RED if r[2] else BLUE for r in rows]
    y = list(range(len(rows)))[::-1]

    ax.barh(y, values, height=0.62, color=colors, zorder=3)

    top = max(values) * 1.13
    ax.set_xlim(0, top)
    ax.set_yticks(y)
    ax.set_yticklabels([r[0] for r in rows], fontsize=9, color="#202124")
    ax.set_xlabel(xlabel, fontsize=9, color=GREY, labelpad=8)
    ax.tick_params(axis="x", labelsize=8.5, colors=GREY, length=0)
    ax.tick_params(axis="y", length=0)

    for spine in ("top", "right", "left"):
        ax.spines[spine].set_visible(False)
    ax.spines["bottom"].set_color(LIGHT)
    ax.xaxis.grid(True, color=LIGHT, linewidth=0.8, zorder=0)
    ax.set_axisbelow(True)

    for yi, value in zip(y, values):
        label = "{:,.2f}".format(value)
        # inside the bar when it fits there, just outside when it does not
        if value > top * 0.22:
            ax.text(value - top * 0.012, yi, label, va="center", ha="right", color="white", fontsize=9, zorder=4)
        else:
            ax.text(value + top * 0.012, yi, label, va="center", ha="left", color=GREY, fontsize=9, zorder=4)

    # both flush with the left edge of the image, not of the axes
    fig.text(0.018, 1 - 0.36 / fig_h, title, ha="left", va="top", fontsize=13.5, color="#202124")
    fig.text(0.018, 1 - 0.72 / fig_h, subtitle, ha="left", va="top", fontsize=8.5, color=GREY)

    fig.tight_layout(rect=(0, 0, 1, 1 - 0.92 / fig_h))
    fig.savefig(path, facecolor="white")
    plt.close(fig)
    print("wrote %s" % path)


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("binary", help="the bench-solo executable, built with NDEBUG")
    p.add_argument("outdir", help="where the png files go, doc/ for the README")
    p.add_argument("-n", "--runs", type=int, default=5, help="processes per container per workload")
    p.add_argument("--cpu", default=None, help="pin to this cpu with taskset, e.g. 4")
    args = p.parse_args()

    workloads = [c[0] for c in CHARTS] + EXTRA
    results = {}
    for workload in workloads:
        print("%s ..." % workload, flush=True)
        results[workload] = {}
        for index, name in CONTAINERS:
            got = measure(args.binary, workload, index, args.runs, args.cpu)
            if got is not None:
                results[workload][index] = got

    for workload in workloads:
        print("\n### %s" % workload)
        for index, name in CONTAINERS:
            v = results[workload].get(index)
            if v is None:
                print("  %-24s not built in" % name.replace("\n", ""))
                continue
            print("  %-24s %10.2f ns %10.1f ins   spread %.1f%%"
                  % (name.replace("\n", ""), v["ns"], v["ins"], v["spread"] * 100))

    print()
    for workload, filename, title, subtitle, xlabel, scale in CHARTS:
        rows = [(name.replace("\n", "\n"), results[workload][i]["ns"] * scale, i == 3)
                for i, name in CONTAINERS if i in results[workload]]
        render(os.path.join(args.outdir, filename), title, subtitle, xlabel, rows)


if __name__ == "__main__":
    main()
